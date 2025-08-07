#pragma once
#include "include/ICrawler.h"
#include <curl/curl.h>

namespace purecpp::websearch {

/**
 * Implementação baseada em libcurl (HTTP/2 habilitado quando possível).
 */
class CurlCrawler : public ICrawler {
public:
    CurlCrawler();
    ~CurlCrawler() override;

    std::string fetch(const std::string& url,
                      std::string& mime_out,
                      int timeout_ms = 8000) override;

private:
    CURLM* multi_;      // conexão reutilizada entre chamadas
};

} // namespace purecpp::websearch
