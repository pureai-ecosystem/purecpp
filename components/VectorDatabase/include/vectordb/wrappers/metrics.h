#pragma once
/**
 * MetricsWrapper
 * --------------
 * Collects usage statistics (count, errors, latency) for any
 * `VectorBackend`. Fully thread-safe.
 *
 *  • Does not add external dependencies – only uses the STL.
 *  • Expose metrics via `snapshot()` (C++ map) or
 *    `snapshot_json()` (JSON string using nlohmann/json).
 *  • If you want Prometheus support, it's easy to plug in later (see TODO in .cpp).
 */

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "vectordb/backend.h"
#include "CommonStructs.h"

namespace vdb::wrappers {

struct CallStats {
    std::uint64_t calls   = 0;
    std::uint64_t errors  = 0;
    double        total   = 0.0;         
    double        min_t   = std::numeric_limits<double>::infinity();
    double        max_t   = 0.0;

    void observe(double elapsed, bool ok) noexcept;
};


class MetricsWrapper final : public VectorBackend {
public:
    explicit MetricsWrapper(VectorBackendPtr backend,
                            std::string       namespace_ = "vdb");

    [[nodiscard]] bool is_open() const noexcept override;
    void insert(std::span<const RAGLibrary::Document> docs) override;
    std::vector<QueryResult>
    query(std::span<const float>               embedding,
          std::size_t                         k,
          const std::unordered_map<std::string,std::string>* filter = nullptr) override;

    void close() override;

    [[nodiscard]]
    std::unordered_map<std::string, CallStats> snapshot() const;

    [[nodiscard]] std::string snapshot_json(int indent = 2) const;

    void reset();

private:
    template<typename F, typename... Args>
    auto measure(const std::string& method, F&& fn, Args&&... args);

    VectorBackendPtr                                       backend_;
    mutable std::mutex                                     mtx_;
    std::unordered_map<std::string, CallStats>             stats_;
    const std::string                                      ns_;     
};

}  // namespace vdb::wrappers
