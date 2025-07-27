#include "vectordb/document.h"
#include <nlohmann/json.hpp>

namespace vdb {

std::string Document::to_json() const {
    nlohmann::json j = {
        {"page_content", page_content},
        {"embedding",    embedding},
        {"metadata",     metadata},
    };
    return j.dump();              
}

Document Document::from_json(std::string_view js) {
    auto j = nlohmann::json::parse(js);

    Document d;
    d.page_content = j.at("page_content").get<std::string>();
    d.embedding    = j.at("embedding"   ).get<std::vector<float>>();
    d.metadata     = j.value("metadata",                   // opcional
                             std::unordered_map<std::string,std::string>{});
    return d;
}

} // namespace vdb
