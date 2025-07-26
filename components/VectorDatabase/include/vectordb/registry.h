#pragma once

#include "backend.h"

#include <nlohmann/json.hpp>   // <-- garante que nlohmann::json seja visível
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace vdb {

class Registry {
public:
    using Factory = std::function<VectorBackendPtr(const nlohmann::json&)>;

    static Registry& instance();

    void register_backend(const std::string& name,
                          Factory factory,
                          bool allow_override = false);

    [[nodiscard]] VectorBackendPtr make(const std::string& name,
                                        const nlohmann::json& cfg) const;

    [[nodiscard]] std::vector<std::string> list() const;

private:
    Registry() = default;
    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;

    mutable std::mutex               mtx_;
    std::map<std::string, Factory>   factories_;
};

template <class Concrete>
struct AutoRegister {
    explicit AutoRegister(const std::string& name) {
        Registry::instance().register_backend(
            name,
            [](const nlohmann::json& cfg) {
                return std::make_unique<Concrete>(cfg);
            });
    }
};

} // namespace vdb
