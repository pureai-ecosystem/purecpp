#include "vectordb/backend.h"
#include "vectordb/registry.h"
#include "vectordb/exceptions.h"

#include <odpi.hpp>                 
#include <nlohmann/json.hpp>

#include <random>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <optional>
#include <span>


#ifndef DPI_CPP_WRAPPER_FOUND        
namespace dpi {

struct Exception : std::runtime_error { using std::runtime_error::runtime_error; };
struct PoolCreateParams {};

class Conn {
public:
    void executeImmediate(const std::string&) {}
    void commit()   {}
    void rollback() {}
};

class Pool {
public:
    static Pool create(const std::string&, const std::string&, const std::string&,
                       int, int, int, const PoolCreateParams&) { return {}; }
    bool isValid()  const noexcept { return true; }
    Conn acquire()  const          { return {}; }
    void close()                    {}
};

struct Query {
    bool next() { return false; }
    std::string getClob  (int) { return {}; }
    std::string getString(int) { return {}; }
    double      getDouble(int) { return 0.0; }
};

class Stmt {
public:
    Stmt(const Conn&, const std::string&) {}
    void setBatch(std::size_t) {}
    void bindStrings (int,               const std::string*, std::size_t) {}
    void bindClobs   (int,               const std::string*, std::size_t) {}
    void bindBytes   (int,               const void*,        std::size_t) {}
    void bindBytes   (const std::string&, const void*,        std::size_t) {}
    void bindUint32  (int,               uint32_t) {}
    void bindUint32  (const std::string&, uint32_t) {}
    void bindString  (const std::string&, const std::string&) {}
    Query executeQuery() { return {}; }
    void  executeMany () {}
};

struct Context { static void init() {} };

} // namespace dpi
#endif

namespace vdb {

static std::string gen_uuid()
{
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<uint64_t> dist;
    uint64_t a = dist(rng), b = dist(rng);
    a = (a & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
    b = (b & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

    std::ostringstream ss;
    ss << std::hex << std::setfill('0')
       << std::setw(8)  << (a >> 32)           << "-"
       << std::setw(4)  << ((a >> 16) & 0xFFFF) << "-"
       << std::setw(4)  << (a & 0xFFFF)        << "-"
       << std::setw(4)  << (b >> 48)           << "-"
       << std::setw(12) << (b & 0xFFFFFFFFFFFFULL);
    return ss.str();
}

static std::vector<uint8_t> to_raw(std::span<const float> v)
{
    const auto* p = reinterpret_cast<const uint8_t*>(v.data());
    return {p, p + v.size() * sizeof(float)};
}

/* ---------- backend ---------- */
class OracleVectorBackend final : public VectorBackend {
public:
    explicit OracleVectorBackend(const nlohmann::json& cfg)
        : VectorBackend(cfg.at("dim").get<uint32_t>())
        , user_    (cfg.at("user").get<std::string>())
        , pwd_     (cfg.at("password").get<std::string>())
        , connStr_ (cfg.at("connect_string").get<std::string>())
        , table_   (cfg.value("table" , "VDB_DOCS"))
        , metric_  (cfg.value("metric", "COSINE"))          // COSINE | L2 | IP
        , idx_     (cfg.value("index",  table_ + "_VEC_IDX"))
        , poolSize_(cfg.value("pool_size", 4))
    {
        dpi::Context::init();
        pool_ = dpi::Pool::create(user_, pwd_, connStr_,
                                  poolSize_, poolSize_, 1,
                                  dpi::PoolCreateParams{});
        ensure_schema();
    }

    bool is_open() const noexcept override { return pool_.isValid(); }

    void insert(std::span<const RAGLibrary::Document> docs) override
    {
        if (docs.empty()) return;
        auto conn = pool_.acquire();

        dpi::Stmt stmt(conn,
            "INSERT INTO " + table_ +
            "(id,page_content,metadata,embedding) VALUES (:1,:2,:3,ai_vector(:4))");
        stmt.setBatch(docs.size());

        std::vector<std::string> ids, pages, metas;
        std::vector<std::vector<uint8_t>> raws;
        ids.reserve(docs.size());
        pages.reserve(docs.size());
        metas.reserve(docs.size());
        raws.reserve(docs.size());

        for (const auto& d : docs) {
            if (!d.embedding || d.embedding->size() != dim_)
                throw DimensionMismatch("embedding ausente ou dimensão incorreta");

            ids  .push_back(gen_uuid());
            pages.push_back(d.page_content);
            metas.push_back(nlohmann::json(d.metadata).dump());
            raws .push_back(to_raw(*d.embedding));
        }

        stmt.bindStrings (1, ids  .data(), ids  .size());
        stmt.bindClobs   (2, pages.data(), pages.size());
        stmt.bindStrings (3, metas.data(), metas.size());
        stmt.bindBytes   (4, raws .data(), raws .size());

        try {
            stmt.executeMany();
            conn.commit();
        } catch (const dpi::Exception& e) {
            conn.rollback();
            throw InsertionError(e.what());
        }
    }

    std::vector<QueryResult>
    query(std::span<const float> emb, std::size_t k,
          const std::unordered_map<std::string,std::string>* filter = nullptr) override
    {
        if (emb.size() != dim_) throw DimensionMismatch("dimensão incorreta");
        auto conn = pool_.acquire();

        std::ostringstream sql;
        sql << "SELECT page_content, metadata, "
            << "vector_distance_" << metric_
            << "(embedding, ai_vector(:vec)) dist "
            << "FROM " << table_;

        if (filter && !filter->empty()) {
            sql << " WHERE ";
            bool first = true;
            for (auto& [f, v] : *filter) {
                if (!first) sql << " AND ";
                first = false;
                sql << "JSON_VALUE(metadata,'$." << f << "') = :" << f;
            }
        }
        sql << " ORDER BY dist FETCH FIRST :k ROWS ONLY";

        dpi::Stmt stmt(conn, sql.str());
        auto raw = to_raw(emb);
        stmt.bindBytes (":vec", raw.data(), raw.size());
        stmt.bindUint32(":k",   static_cast<uint32_t>(k));

        if (filter) {
            for (auto& [f, v] : *filter) stmt.bindString(f, v);
        }

        std::vector<QueryResult> out;
        dpi::Query q = stmt.executeQuery();
        while (q.next()) {
            std::string page = q.getClob  (1);
            std::string meta = q.getString(2);
            double      dist = q.getDouble(3);

            float score = (metric_ == "COSINE")
                          ? static_cast<float>(1.0 - dist * 0.5)
                          : static_cast<float>(dist);

            RAGLibrary::Document d = RAGLibrary::Document::from_json(meta);
            d.page_content = std::move(page);
            out.push_back({std::move(d), score});
        }
        return out;
    }

    void close() override { if (is_open()) pool_.close(); }

private:
    void ensure_schema()
    {
        auto conn = pool_.acquire();

        {
            std::ostringstream ss;
            ss << "BEGIN\n"
                  " EXECUTE IMMEDIATE 'CREATE TABLE " << table_ << " (\n"
                  "   id           VARCHAR2(36) PRIMARY KEY,\n"
                  "   page_content CLOB,\n"
                  "   metadata     JSON,\n"
                  "   embedding    VECTOR(" << dim_ << ")\n"
                  " )';\n"
                  " EXCEPTION WHEN OTHERS THEN IF SQLCODE != -955 THEN RAISE; END IF;\n"
                  "END;";
            conn.executeImmediate(ss.str());
        }

        {
            std::ostringstream ss;
            ss << "DECLARE\n"
                  " e_exists EXCEPTION; PRAGMA EXCEPTION_INIT(e_exists,-955);\n"
                  "BEGIN\n"
                  " DBMS_VECTOR.CREATE_INDEX(\n"
                  "   index_name      => '" << idx_    << "',\n"
                  "   table_name      => '" << table_  << "',\n"
                  "   column_name     => 'EMBEDDING',\n"
                  "   distance_metric => '" << metric_ << "',\n"
                  "   index_type      => 'HNSW');\n"
                  " EXCEPTION WHEN e_exists THEN NULL;\n"
                  "END;";
            conn.executeImmediate(ss.str());
        }
    }

    
    std::string user_, pwd_, connStr_, table_, metric_, idx_;
    int         poolSize_{4};
    dpi::Pool   pool_;
};


static AutoRegister<OracleVectorBackend> _reg_oracle("oracle");
void force_link_oracle_backend() { (void)_reg_oracle; }

} // namespace vdb
