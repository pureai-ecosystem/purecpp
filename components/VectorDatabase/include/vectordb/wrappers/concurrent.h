#pragma once
/**
 * ConcurrentSearchWrapper
 * -----------------------
 * A thin, thread-pool façade around any VectorBackend.
 *
 *  • Inserts are serialised (they mutate state).
 *  • Queries can run in parallel as long as the wrapped backend
 *    is thread-safe for `query()`; otherwise we serialise them too.
 *
 * Build-only dependency: <future> / <thread> (no Boost/TBB needed).
 */
#include <future>
#include <mutex>
#include <span>
#include <thread>
#include <unordered_map>
#include <vector>

#include "vectordb/backend.h"

namespace vdb::wrappers {

class ConcurrentSearchWrapper final : public VectorBackend {
public:
    /** 
     * @param backend          Ownership is transferred (unique_ptr)
     * @param maxWorkers       #threads in the internal pool (0 → hw concurrency)
     * @param backendThreadSafe If false we serialise every call to query()
     */
    explicit ConcurrentSearchWrapper(VectorBackendPtr backend,
                                     std::size_t      maxWorkers        = std::thread::hardware_concurrency(),
                                     bool             backendThreadSafe = true);

    ~ConcurrentSearchWrapper() override;

    [[nodiscard]] bool is_open() const noexcept override;
    void insert(std::span<const Document> docs) override;

    std::vector<QueryResult>
    query(std::span<const float>               embedding,
          std::size_t                         k,
          const std::unordered_map<std::string, std::string>* filter = nullptr) override;

    std::vector<std::vector<QueryResult>>
    query_many(const std::vector<std::vector<float>>&            embeddings,
               std::size_t                                       k           = 5,
               const std::unordered_map<std::string, std::string>* filter    = nullptr,
               bool                                              raiseOnErr = false);

    void close() override;

private:
    VectorBackendPtr backend_;
    std::size_t      workers_;
    bool             backendThreadSafe_;
    std::mutex       mtx_;        // protects non-thread-safe back-ends
};

}  // namespace vdb::wrappers
