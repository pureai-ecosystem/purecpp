#include "src/crawler/CurlCrawler.h"
#include <stdexcept>
#include <vector>

namespace purecpp::websearch {

/* ---------- helpers -------------------------------------------------- */

static size_t write_cb(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    auto* buf = static_cast<std::string*>(userdata);
    buf->append(ptr, size * nmemb);
    return size * nmemb;
}

/* ---------- ctor/dtor ------------------------------------------------ */

CurlCrawler::CurlCrawler()
{
    curl_global_init(CURL_GLOBAL_ALL);
    multi_ = curl_multi_init();
    curl_multi_setopt(multi_, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);
}

CurlCrawler::~CurlCrawler()
{
    curl_multi_cleanup(multi_);
    curl_global_cleanup();
}

/* ---------- fetch ---------------------------------------------------- */

std::string CurlCrawler::fetch(const std::string& url,
                               std::string& mime_out,
                               int timeout_ms)
{
    CURL* easy = curl_easy_init();
    if (!easy) throw std::runtime_error("curl_easy_init failed");

    std::string buffer;
    curl_easy_setopt(easy, CURLOPT_URL, url.c_str());
    curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(easy, CURLOPT_ACCEPT_ENCODING, "");          // any
    curl_easy_setopt(easy, CURLOPT_USERAGENT,
                     "PureCPP-WebSearch/1.0 (+https://pure.ai)");
    curl_easy_setopt(easy, CURLOPT_TIMEOUT_MS, timeout_ms);
    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(easy, CURLOPT_WRITEDATA, &buffer);

    curl_multi_add_handle(multi_, easy);

    int running = 0;
    do {
        CURLMcode mc = curl_multi_perform(multi_, &running);
        if (mc != CURLM_OK)
            throw std::runtime_error("curl_multi_perform failed");
        /* wait for activity, max 100 ms */
        curl_multi_poll(multi_, nullptr, 0, 100, nullptr);
    } while (running);

    /* check HTTP status */
    long status = 0;
    curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &status);
    if (status >= 400)
        throw std::runtime_error("HTTP " + std::to_string(status) + " for " + url);

    /* mime */
    char* ct = nullptr;
    curl_easy_getinfo(easy, CURLINFO_CONTENT_TYPE, &ct);
    if (ct) mime_out = ct;

    curl_multi_remove_handle(multi_, easy);
    curl_easy_cleanup(easy);

    return buffer;
}

} // namespace purecpp::websearch
