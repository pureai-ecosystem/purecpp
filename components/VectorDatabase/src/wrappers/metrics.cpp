#include "vectordb/wrappers/metrics.h"
#include "vectordb/exceptions.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <functional>
#include <type_traits>
#include <utility>

using namespace std::chrono;

namespace vdb::wrappers {

/* ----------------------------- CallStats ------------------- */
void CallStats::observe(double elapsed, bool ok) noexcept {
    ++calls;
    if (!ok) ++errors;
    total += elapsed;
    min_t  = std::min(min_t, elapsed);
    max_t  = std::max(max_t, elapsed);
}

/* ----------------------------- ctor ------------------------ */
MetricsWrapper::MetricsWrapper(VectorBackendPtr backend, std::string ns)
    : VectorBackend(backend->dim())
    , backend_(std::move(backend))
    , ns_(std::move(ns)) {}

/* ----------------------------- status ---------------------- */
bool MetricsWrapper::is_open() const noexcept { return backend_->is_open(); }

/* ----------------------------- measure() ------------------- */
template <typename F, typename... Args>
auto MetricsWrapper::measure(const std::string& method, F&& fn, Args&&... args) {
    const auto start = steady_clock::now();
    bool ok = true;

    using R = std::invoke_result_t<F, Args...>;
    try {
        if constexpr (std::is_void_v<R>) {
            std::invoke(std::forward<F>(fn), std::forward<Args>(args)...);
            const auto elapsed = duration<double>(steady_clock::now() - start).count();
            std::scoped_lock g(mtx_);
            stats_[method].observe(elapsed, ok);
            return; // void
        } else {
            R result = std::invoke(std::forward<F>(fn), std::forward<Args>(args)...);
            const auto elapsed = duration<double>(steady_clock::now() - start).count();
            std::scoped_lock g(mtx_);
            stats_[method].observe(elapsed, ok);
            return result;
        }
    } catch (...) {
        ok = false;
        const auto elapsed = duration<double>(steady_clock::now() - start).count();
        {
            std::scoped_lock g(mtx_);
            stats_[method].observe(elapsed, ok);
        }
        throw;
    }
}

/* ----------------------------- insert ---------------------- */
void MetricsWrapper::insert(std::span<const Document> docs) {
    measure("insert", &VectorBackend::insert, backend_.get(), docs);
}

/* ----------------------------- query ----------------------- */
std::vector<QueryResult>
MetricsWrapper::query(std::span<const float> emb,
                      std::size_t            k,
                      const std::unordered_map<std::string, std::string>* filter) {
    return measure("query", &VectorBackend::query, backend_.get(), emb, k, filter);
}

/* ----------------------------- close ----------------------- */
void MetricsWrapper::close() {
    measure("close", &VectorBackend::close, backend_.get());
}

/* ----------------------------- snapshot -------------------- */
std::unordered_map<std::string, CallStats> MetricsWrapper::snapshot() const {
    std::scoped_lock g(mtx_);
    return stats_;
}

std::string MetricsWrapper::snapshot_json(int indent) const {
    nlohmann::json j;
    auto snap = snapshot();
    for (auto& [k, s] : snap) {
        j[k] = {
            {"calls",  s.calls},
            {"errors", s.errors},
            {"total",  s.total},
            {"min",    std::isinf(s.min_t) ? nlohmann::json(nullptr) : nlohmann::json(s.min_t)},
            {"max",    s.max_t},
            {"avg",    s.calls ? s.total / s.calls : 0.0}
        };
    }
    return j.dump(indent);
}

void MetricsWrapper::reset() {
    std::scoped_lock g(mtx_);
    stats_.clear();
}

} // namespace vdb::wrappers
