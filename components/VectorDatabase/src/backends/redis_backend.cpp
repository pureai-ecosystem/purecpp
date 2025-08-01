// components/VectorDatabase/src/backends/redis_backend.cpp
#include "vectordb/backend.h"
#include "vectordb/registry.h"
#include "vectordb/exceptions.h"

#include <nlohmann/json.hpp>
#include <sw/redis++/redis++.h>
#include <hiredis/hiredis.h>

#include <random>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <memory>

#include "CommonStructs.h"

namespace vdb {

class RedisVectorBackend final : public VectorBackend {
    using redis_t = sw::redis::Redis;

public:
    explicit RedisVectorBackend(const nlohmann::json& cfg)
        : VectorBackend(cfg.at("dim").get<std::uint32_t>())
        , redis_(std::make_shared<redis_t>(cfg.value("uri", "tcp://127.0.0.1:6379")))
        , index_(cfg.value("index",  "vstore_idx"))
        , prefix_(cfg.value("prefix", "doc"))
        , metric_(cfg.value("metric", "COSINE"))  // "COSINE" | "L2" | "IP"
    {
        ensure_index(cfg.value("capacity", 0));
    }

    bool is_open() const noexcept override { return static_cast<bool>(redis_); }

    void insert(std::span<const RAGLibrary::Document> docs) override {
        if (!is_open()) throw BackendClosed("Redis backend closed");

        auto pipe = redis_->pipeline(false); // non-atomic

        for (const auto& d : docs) {
            if (d.dim() != dim_)
                throw DimensionMismatch("Dimension mismatch on insert");
    
            if (!d.embedding.has_value())  // check if has embedding
                throw InsertionError("Document missing embedding data");

            const auto& embedding = *d.embedding;
            const std::string key = prefix_ + ":" + gen_uuid();

            const std::string binary(reinterpret_cast<const char*>(embedding.data()),
                                     embedding.size() * sizeof(float));

            pipe.hset(key, std::initializer_list<std::pair<std::string,std::string>>{
                {"vector",   binary},
                {"page",     d.page_content},
                {"metadata", d.to_json()}
            });
        }

        try {
            pipe.exec();
        } catch (const sw::redis::Error& e) {
            throw InsertionError(e.what());
        }
    }

    std::vector<QueryResult>
    query(std::span<const float> embedding,
          std::size_t k,
          const std::unordered_map<std::string, std::string>* filter) override {

        if (embedding.size() != dim_)
            throw DimensionMismatch("Dimension mismatch on query");

        const std::string vec_bin(reinterpret_cast<const char*>(embedding.data()),
                                  embedding.size() * sizeof(float));

        std::string base = "*";
        if (filter && !filter->empty()) {
            std::string f;
            for (const auto& [field, value] : *filter) {
                (void)field; 
                f += "@metadata:\"" + value + "\" ";
            }
            base = std::move(f);
        }

        const std::string knn_clause =
            "=>[KNN " + std::to_string(k) + " @vector $vec AS score]";
        const std::string q = base + " " + knn_clause;

        std::vector<std::string> argv = {
            "FT.SEARCH",
            index_,
            q,
            "PARAMS", "2", "vec", vec_bin,
            "RETURN", "4", "page", "metadata", "vector", "score", 
            "DIALECT", "2",
            "SORTBY", "score", "ASC"
        };

        sw::redis::ReplyUPtr reply;
        try {
            reply = redis_->command(argv.begin(), argv.end());
        } catch (const sw::redis::Error& e) {
            throw QueryError(e.what());
        }

        return parse_search_reply_tree(reply.get());
    }

    void close() override { redis_.reset(); }

private:
    std::shared_ptr<redis_t> redis_;
    std::string              index_, prefix_, metric_;


    static std::string safe_str(const redisReply* r) {
        return (r && r->str) ? std::string(r->str, r->len) : std::string();
    }

    static std::string gen_uuid() {
        static thread_local std::mt19937_64 rng{std::random_device{}()};
        static constexpr char hex[] = "0123456789abcdef";
        std::uniform_int_distribution<uint64_t> dist;

        auto to_hex = [&](uint64_t x, int bytes) {
            std::string out;
            out.reserve(bytes * 2);
            for (int i = bytes * 2 - 1; i >= 0; --i)
                out.push_back(hex[(x >> (i * 4)) & 0xF]);
            return out;
        };

        uint64_t a = dist(rng);
        uint64_t b = dist(rng);

        a = (a & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
        b = (b & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

        std::string s = to_hex(a >> 32, 4) + "-" +
                        to_hex(a & 0xFFFFFFFFULL, 4).substr(0,4) + "-" +
                        to_hex(a & 0xFFFFFFFFULL, 4).substr(4,4) + "-" +
                        to_hex(b >> 48, 2) + "-" +
                        to_hex(b & 0x0000FFFFFFFFFFFFULL, 6);
        return s;
    }

    void ensure_index(int capacity) {
        try {
            std::vector<std::string> info_args = {"FT.INFO", index_};
            auto r = redis_->command(info_args.begin(), info_args.end());
            (void)r;
            return; 
        } catch (const sw::redis::Error&) {
            // create
        }

        const std::string dim_str    = std::to_string(dim_);
        const std::string algo       = "FLAT"; // or HNSW
        const std::string cmd_metric = metric_;

        std::vector<std::string> args = {
            "FT.CREATE", index_,
            "ON", "HASH",
            "PREFIX", "1", prefix_,
            "SCHEMA",
            "vector", "VECTOR", algo,
            "6",
            "TYPE", "FLOAT32",
            "DIM", dim_str,
            "DISTANCE_METRIC", cmd_metric,
            "page", "TEXT",
            "metadata", "TEXT"
        };

        if (capacity > 0 && algo == "HNSW") {
            args.push_back("INITIAL_CAP");
            args.push_back(std::to_string(capacity));
        }

        try {
            redis_->command(args.begin(), args.end());
        } catch (const sw::redis::Error& e) {
            throw InvalidConfiguration(std::string("FT.CREATE falhou: ") + e.what());
        }
    }

    static std::vector<QueryResult> parse_search_reply_tree(const redisReply* root) {
        std::vector<QueryResult> out;
        if (!root || root->type != REDIS_REPLY_ARRAY || root->elements == 0)
            return out;

        const redisReply* count_r = root->element[0];
        if (!count_r) return out;
        std::size_t total = 0;
        if (count_r->type == REDIS_REPLY_INTEGER) {
            total = static_cast<std::size_t>(count_r->integer);
        } else if (count_r->type == REDIS_REPLY_STRING) {
            total = std::stoull(safe_str(count_r));
        } else {
            return out;
        }

        std::size_t pos = 1;
        out.reserve(total);

        for (std::size_t i = 0; i < total && pos + 1 < root->elements; ++i) {
            const redisReply* key_r  = root->element[pos++];
            const redisReply* arr_r  = root->element[pos++];

            (void)key_r; 

            if (!arr_r || arr_r->type != REDIS_REPLY_ARRAY) continue;

            std::string page, metadata_json, vector_bin;
            float score = 0.f;

            for (size_t j = 0; j + 1 < arr_r->elements; j += 2) {
                const redisReply* f = arr_r->element[j];
                const redisReply* v = arr_r->element[j+1];
                std::string field = safe_str(f);
                std::string value = safe_str(v);

                if      (field == "page")     page = std::move(value);
                else if (field == "metadata") metadata_json = std::move(value);
                else if (field == "vector")   vector_bin = std::move(value);
                else if (field == "score") {
                    try { score = std::stof(value); } catch(...) { score = 0.f; }
                }
            }

            std::vector<float> emb;
            if (!vector_bin.empty()) {
                emb.resize(vector_bin.size() / sizeof(float));
                std::memcpy(emb.data(), vector_bin.data(), vector_bin.size());
            }

            auto meta = RAGLibrary::Document::from_json(metadata_json).metadata;
            RAGLibrary::Document doc{std::move(meta), page, std::move(emb)};
            out.push_back(QueryResult{std::move(doc), score});
        }

        return out;
    }
};

static AutoRegister<RedisVectorBackend> _auto_register_redis("redis");

void force_link_redis_backend() {
    (void)_auto_register_redis;
}
} // namespace vdb
