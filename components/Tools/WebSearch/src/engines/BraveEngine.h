#pragma once
#include "include/IWebSearch.h"
#include <string>
#include <vector>

namespace purecpp::websearch {

/**
 *  Brave Search API (https://api.search.brave.com/)
 *  Requer BRAVE_SEARCH_KEY no env (ou setter no ctor).
 */
class BraveEngine : public IWebSearch {
public:
    explicit BraveEngine(std::string api_key = {});

    std::vector<UrlResult> query(const std::string& q,
                                 int                k,
                                 const std::string& lang = {}) override;

private:
    std::string key_;
};

} // namespace purecpp::websearch
