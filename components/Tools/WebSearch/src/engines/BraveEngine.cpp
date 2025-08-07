#include "src/engines/BraveEngine.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <sstream>

namespace purecpp::websearch {
using nlohmann::json;

/* ------------ helpers ------------------------------------------------ */

static size_t write_cb(char* ptr, size_t sz, size_t nm, void* ud)
{
    auto* buf = static_cast<std::string*>(ud);
    buf->append(ptr, sz * nm);
    return sz * nm;
}

/* ------------ ctor --------------------------------------------------- */

BraveEngine::BraveEngine(std::string api_key)
{
    if (api_key.empty()) {
        const char* env = std::getenv("BRAVE_SEARCH_KEY");
        if (!env) throw std::runtime_error("BRAVE_SEARCH_KEY not set");
        api_key = env;
    }
    key_ = std::move(api_key);
}

/* ------------ query -------------------------------------------------- */

std::vector<UrlResult> BraveEngine::query(const std::string& q,
                                          int                k,
                                          const std::string& lang)
{
    std::ostringstream url;
    url << "https://api.search.brave.com/res/v1/web/search?q="
        << curl_easy_escape(nullptr, q.c_str(), 0)
        << "&count=" << k
        << "&source=web";
    if (!lang.empty()) url << "&lang=" << lang;

    CURL* c = curl_easy_init();
    if (!c) throw std::runtime_error("curl_easy_init");

    std::string buf;
    curl_easy_setopt(c, CURLOPT_URL, url.str().c_str());
    curl_easy_setopt(c, CURLOPT_TIMEOUT_MS, 6000);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &buf);
    struct curl_slist* hdrs = nullptr;
    std::string auth = "x-primetime-key: " + key_;
    hdrs = curl_slist_append(hdrs, auth.c_str());
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, hdrs);

    CURLcode rc = curl_easy_perform(c);
    curl_slist_free_all(hdrs);
    curl_easy_cleanup(c);
    if (rc != CURLE_OK) throw std::runtime_error("Brave API error");

    auto j = json::parse(buf);
    std::vector<UrlResult> out;
    int rank = 0;
    for (auto& item : j["web"]["results"]) {
        out.push_back({item["url"].get<std::string>(), rank++});
        if (rank >= k) break;
    }
    return out;
}

} // namespace purecpp::websearch
