#include "CommonStructs.h"
#include <nlohmann/json.hpp>

namespace RAGLibrary
{

    std::string Document::to_json() const
    {
        nlohmann::json j;
        j["page_content"] = page_content;
        if (embedding.has_value())
        {
            j["embedding"] = *embedding;
        }
        else
        {
            j["embedding"] = nullptr;
        }
        j["metadata"] = metadata;
        return j.dump();
    }

    Document Document::from_json(std::string_view js)
    {
        auto j = nlohmann::json::parse(js);

        Document d;
        d.page_content = j.at("page_content").get<std::string>();

        if (j.contains("embedding") && !j["embedding"].is_null())
        {
            d.embedding = j["embedding"].get<std::vector<float>>();
        }
        else
        {
            d.embedding = std::nullopt;
        }

        d.metadata = j.value("metadata", std::map<std::string, std::string>{});
        return d;
    }
}
