#pragma once
/**
 * MetricsWrapper
 * --------------
 * Coleta estatísticas de uso (contagem, erros, latência) de qualquer
 * `VectorBackend`.  Tudo thread-safe.
 *
 *  • Não adiciona dependências externas – só STL.
 *  • Exponha as métricas via `snapshot()` (C++ map) ou
 *    `snapshot_json()` (string JSON usando nlohmann/json).
 *  • Se quiser Prometheus, é fácil plugar depois (ver TODO no .cpp).
 */
#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "vectordb/backend.h"

namespace vdb::wrappers {

struct CallStats {
    std::uint64_t calls   = 0;
    std::uint64_t errors  = 0;
    double        total   = 0.0;          // segundos acumulados
    double        min_t   = std::numeric_limits<double>::infinity();
    double        max_t   = 0.0;

    void observe(double elapsed, bool ok) noexcept;
};

/**
 * Wrapper propriamente dito.
 */
class MetricsWrapper final : public VectorBackend {
public:
    explicit MetricsWrapper(VectorBackendPtr backend,
                            std::string       namespace_ = "vdb");

    /* ------------- VectorBackend interface ----------------- */
    [[nodiscard]] bool is_open() const noexcept override;
    void insert(std::span<const Document> docs) override;
    std::vector<QueryResult>
    query(std::span<const float>               embedding,
          std::size_t                         k,
          const std::unordered_map<std::string,std::string>* filter = nullptr) override;

    /* delega extras se o backend suportar ------------------- */
    void close() override;

    /* ------------- API de métricas ------------------------ */
    [[nodiscard]]
    std::unordered_map<std::string, CallStats> snapshot() const;

    [[nodiscard]] std::string snapshot_json(int indent = 2) const;

    void reset();

private:
    /* util interno que mede + propaga */
    template<typename F, typename... Args>
    auto measure(const std::string& method, F&& fn, Args&&... args);

    VectorBackendPtr                                       backend_;
    mutable std::mutex                                     mtx_;
    std::unordered_map<std::string, CallStats>             stats_;
    const std::string                                      ns_;     // p/ futuramente exp-Prometheus
};

}  // namespace vdb::wrappers
