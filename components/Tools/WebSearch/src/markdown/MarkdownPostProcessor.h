#pragma once
#include <string>

namespace purecpp::websearch::md {

/**
 * Ajustes leves no Markdown já produzido pelo HtmlCleaner.
 *  • remove espaços duplos
 *  • colapsa \n\n\n → \n\n
 *  • trim em cada linha
 *  • força newline final
 *
 *  Não altera tabelas ou blocos de código.
 */
class MarkdownPostProcessor {
public:
    /* processa in-place (retorna referência p/ encadear) */
    std::string& run(std::string& md) const;
};

} // namespace purecpp::websearch::md
