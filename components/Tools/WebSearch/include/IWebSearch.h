#pragma once
#include <string>
#include <vector>

namespace purecpp::websearch {

/* URL + posição no ranking do mecanismo de busca. */
struct UrlResult {
    std::string url;
    int         rank;   // 0-based
};

class IWebSearch {
public:
    virtual ~IWebSearch() = default;

    /**
     * Executa a busca na web.
     * @param query Consulta textual.
     * @param k     Quantidade de resultados desejados.
     * @param lang  ISO-639-1 ("en", "pt", …) ou vazio p/ default.
     * @return      Vetor com até k UrlResult ordenados.
     * @throw       std::runtime_error em caso de erro.
     */
    virtual std::vector<UrlResult> query(const std::string& query,
                                         int k,
                                         const std::string& lang = {}) = 0;
};

} // namespace purecpp::websearch
