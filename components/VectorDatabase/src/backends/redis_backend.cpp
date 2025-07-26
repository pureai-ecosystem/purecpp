// redis_backend.cpp
#include "vectordb/backend.h"
#include "vectordb/registry.h"
#include "vectordb/exceptions.h"
#include "vectordb/document.h"

#include <nlohmann/json.hpp>
#include <sw/redis++/redis++.h>
#include <hiredis/hiredis.h>

#include <random>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <cstring>   // memcpy
#include <stdexcept>

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

    /* ----------------------------- insert ----------------------------- */
    void insert(std::span<const Document> docs) override {
        if (!is_open()) throw BackendClosed("Redis backend closed");

        auto pipe = redis_->pipeline(false); // non-atomic for throughput

        for (const auto& d : docs) {
            if (d.dim() != dim_) throw DimensionMismatch("Dimension mismatch on insert");

            const std::string key = prefix_ + ":" + gen_uuid();

            // serialize embedding
            const std::string binary(reinterpret_cast<const char*>(d.embedding.data()),
                                     d.embedding.size() * sizeof(float));

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

    /* ------------------------------ query ----------------------------- */
    std::vector<QueryResult>
    query(std::span<const float> embedding,
          std::size_t k,
          const std::unordered_map<std::string, std::string>* filter) override {

        if (embedding.size() != dim_) throw DimensionMismatch("Dimension mismatch on query");

        const std::string vec_bin(reinterpret_cast<const char*>(embedding.data()),
                                  embedding.size() * sizeof(float));

        std::string base = "*";
        if (filter && !filter->empty()) {
            std::string f;
            for (const auto& [field, value] : *filter) {
                (void)field; // metadata blob -> contains
                f += "@metadata:\"" + value + "\" ";
            }
            base = std::move(f);
        }

        const std::string knn_clause =
            "=>[KNN " + std::to_string(k) + " @vector $vec AS score]";
        const std::string q = base + " " + knn_clause;

        // Monta argv para FT.SEARCH
        std::vector<std::string> argv = {
            "FT.SEARCH",
            index_,
            q,
            "PARAMS", "2", "vec", vec_bin,
            "RETURN", "3", "page", "metadata", "vector",
            "DIALECT", "2",
            "SORTBY", "score", "ASC"
        };

        sw::redis::ReplyUPtr reply;
        try {
            reply = redis_->command(argv.begin(), argv.end());
        } catch (const sw::redis::Error& e) {
            throw QueryError(e.what());
        }

        std::vector<std::string> flat;
        flatten_reply(reply.get(), flat);
        return parse_search_reply(flat, prefix_);
    }

    void close() override { redis_.reset(); }

private:
    std::shared_ptr<redis_t> redis_;
    std::string              index_, prefix_, metric_;

    /* ------------------------- helpers ------------------------- */

    static void flatten_reply(const redisReply* r, std::vector<std::string>& out) {
        if (!r) return;
        switch (r->type) {
        case REDIS_REPLY_STRING:
        case REDIS_REPLY_STATUS:
        case REDIS_REPLY_ERROR:
            out.emplace_back(r->str ? r->str : "");
            break;
        case REDIS_REPLY_INTEGER:
            out.emplace_back(std::to_string(r->integer));
            break;
        case REDIS_REPLY_NIL:
            out.emplace_back(""); // representa NIL como string vazia
            break;
        case REDIS_REPLY_ARRAY:
            for (size_t i = 0; i < r->elements; ++i)
                flatten_reply(r->element[i], out);
            break;
#if HIREDIS_MAJOR >= 1
        case REDIS_REPLY_DOUBLE:
            out.emplace_back(std::to_string(r->dval));
            break;
        case REDIS_REPLY_BOOL:
            out.emplace_back(r->integer ? "1" : "0");
            break;
        case REDIS_REPLY_VERB:
            out.emplace_back(r->str ? r->str : "");
            break;
#endif
        default:
            out.emplace_back("");
        }
    }

    static std::string gen_uuid() {
        static thread_local std::mt19937_64 rng{std::random_device{}()};
        static constexpr char hex[] = "0123456789abcdef";
        std::uniform_int_distribution<uint64_t> dist;

        auto to_hex = [&](uint64_t x, int bytes) {
            std::string out;
            out.reserve(bytes * 2);
            for (int i = bytes * 2 - 1; i >= 0; --i) {
                out.push_back(hex[(x >> (i * 4)) & 0xF]);
            }
            return out;
        };

        uint64_t a = dist(rng);
        uint64_t b = dist(rng);

        // version 4 & variant bits
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
        // FT.INFO para checar existência
        try {
            std::vector<std::string> info;
            auto r = redis_->command(std::vector<std::string>{"FT.INFO", index_}.begin(),
                                     std::vector<std::string>{"FT.INFO", index_}.end());
            flatten_reply(r.get(), info);
            (void)info;
            return;
        } catch (const sw::redis::Error&) {
            // não existe, vamos criar
        }

        std::string dim_str = std::to_string(dim_);
        std::string algo    = "FLAT"; // ou HNSW
        std::string cmd_metric = metric_;

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

    static std::vector<QueryResult>
    parse_search_reply(const std::vector<std::string>& raw, const std::string& prefix) {
        std::vector<QueryResult> out;
        if (raw.empty()) return out;

        std::size_t idx = 0;
        const std::size_t total = std::stoull(raw[idx++]);

        for (std::size_t i = 0; i < total && idx < raw.size(); ++i) {
            const std::string& key = raw[idx++];
            (void)key;

            std::string page, metadata_json, vector_bin;
            float score = 0.f;

            while (idx + 1 < raw.size()) {
                const std::string& field = raw[idx++];
                const std::string& value = raw[idx++];

                if      (field == "page")     page = value;
                else if (field == "metadata") metadata_json = value;
                else if (field == "vector")   vector_bin = value;
                else if (field == "score")    score = std::stof(value);
                else {
                    if (field.rfind(prefix + ":", 0) == 0) { // próximo doc?
                        idx -= 2;
                        break;
                    }
                }

                if (idx < raw.size() && raw[idx].rfind(prefix + ":", 0) == 0)
                    break;
            }

            std::vector<float> emb;
            if (!vector_bin.empty()) {
                emb.resize(vector_bin.size() / sizeof(float));
                std::memcpy(emb.data(), vector_bin.data(), vector_bin.size());
            }

            auto meta = Document::from_json(metadata_json).metadata;
            Document doc{page, std::move(emb), std::move(meta)};
            out.push_back(QueryResult{std::move(doc), score});
        }
        return out;
    }
};

/* -------- auto-registro -------- */
static AutoRegister<RedisVectorBackend> _auto_register_redis("redis");

} // namespace vdb
