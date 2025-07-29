#include "vectordb/registry.h"
#include "vectordb/exceptions.h"

#include <algorithm>
#include <cctype>
#include <mutex>
#include <string>
#include <vector>

namespace {

std::string canonical(std::string n) {
    std::transform(n.begin(), n.end(), n.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return n;
}

} // anonymous namespace

namespace vdb {

Registry& Registry::instance() {
    static Registry inst;
    return inst;
}

void Registry::register_backend(const std::string& name,
                                Factory factory,
                                bool allow_override) {
    const std::string key = canonical(name);
    std::scoped_lock lock(mtx_);

    if (!allow_override && factories_.find(key) != factories_.end()) {
        throw InvalidConfiguration("backend '" + key + "' já registrado");
    }

    factories_[key] = std::move(factory);
}

VectorBackendPtr Registry::make(const std::string& name,
                                const nlohmann::json& cfg) const {
    const std::string key = canonical(name);
    std::scoped_lock lock(mtx_);

    auto it = factories_.find(key);
    if (it == factories_.end()) {
        throw InvalidConfiguration("backend '" + name + "' não encontrado");
    }

    return it->second(cfg);
}

std::vector<std::string> Registry::list() const {
    std::scoped_lock lock(mtx_);
    std::vector<std::string> keys;
    keys.reserve(factories_.size());
    for (const auto& [k, _] : factories_) {
        keys.push_back(k);
    }
    return keys;
}

} // namespace vdb
