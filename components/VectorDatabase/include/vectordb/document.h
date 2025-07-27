#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace vdb {

struct Document {
    std::string                                   page_content;
    std::vector<float>                            embedding;
    std::unordered_map<std::string, std::string>  metadata;

    std::size_t dim() const noexcept { return embedding.size(); }

    [[nodiscard]] std::string to_json() const;
    static Document from_json(std::string_view json);
};

} // namespace vdb
