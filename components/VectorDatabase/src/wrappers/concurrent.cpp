#include "vectordb/wrappers/concurrent.h"
#include "vectordb/exceptions.h"

#include <future>
#include <utility>

namespace vdb::wrappers {

/* -------------------------------------------------- ctor/dtor */
ConcurrentSearchWrapper::ConcurrentSearchWrapper(VectorBackendPtr backend,
                                                 std::size_t      maxWorkers,
                                                 bool             threadSafe)
    : VectorBackend(backend->dim())          // propagate expected dimension
    , backend_(std::move(backend))
    , workers_(maxWorkers ? maxWorkers : 1)  // never zero
    , backendThreadSafe_(threadSafe) {}

ConcurrentSearchWrapper::~ConcurrentSearchWrapper() { close(); }

/* -------------------------------------------------- status  */
bool ConcurrentSearchWrapper::is_open() const noexcept {
    return backend_ && backend_->is_open();
}

/* -------------------------------------------------- insert  */
void ConcurrentSearchWrapper::insert(std::span<const Document> docs) {
    std::scoped_lock g(mtx_);        // always serialise inserts
    backend_->insert(docs);
}

/* -------------------------------------------------- query  */
std::vector<QueryResult>
ConcurrentSearchWrapper::query(std::span<const float>               emb,
                               std::size_t                         k,
                               const std::unordered_map<std::string, std::string>* filter) {
    if (!backendThreadSafe_) {
        std::scoped_lock g(mtx_);
        return backend_->query(emb, k, filter);
    }
    return backend_->query(emb, k, filter);
}

/* -------------------------------------------------- query_many  */
std::vector<std::vector<QueryResult>>
ConcurrentSearchWrapper::query_many(const std::vector<std::vector<float>>&            embs,
                                    std::size_t                                       k,
                                    const std::unordered_map<std::string, std::string>* filter,
                                    bool                                              raiseOnErr) {
    std::vector<std::future<std::vector<QueryResult>>> futs;
    futs.reserve(embs.size());

    /* launch each search in its own async task (simple pool) */
    for (const auto& v : embs) {
        futs.emplace_back(std::async(std::launch::async, [&, vec = v]() {
            return query(vec, k, filter);
        }));
    }

    /* collect preserving order */
    std::vector<std::vector<QueryResult>> out(embs.size());
    for (std::size_t i = 0; i < futs.size(); ++i) {
        try {
            out[i] = futs[i].get();
        } catch (...) {
            if (raiseOnErr) throw;     // propagate first exception
            out[i].clear();            // else: empty result set
        }
    }
    return out;
}

/* -------------------------------------------------- close  */
void ConcurrentSearchWrapper::close() {
    std::scoped_lock g(mtx_);
    if (backend_) {
        backend_->close();
        backend_.reset();
    }
}

}  // namespace vdb::wrappers
