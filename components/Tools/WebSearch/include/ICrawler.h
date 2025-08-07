#pragma once
#include <string>

namespace purecpp::websearch {

/**
 * Responsável por baixar uma URL (HTTP/HTTPS) com políticas de retry.
 */
class ICrawler {
public:
    virtual ~ICrawler() = default;

    /**
     * @param url         URL absoluta a ser baixada.
     * @param mime_out    Recebe o MIME-type detectado/retornado.
     * @param timeout_ms  Timeout por tentativa.
     * @return            Corpo da resposta (string binária / texto).
     * @throw             std::runtime_error se exceder retries ou status != 2xx.
     */
    virtual std::string fetch(const std::string& url,
                              std::string& mime_out,
                              int timeout_ms = 8000) = 0;
};

} // namespace purecpp::websearch
