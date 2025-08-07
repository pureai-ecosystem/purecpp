#pragma once
#include "include/ICleaner.h"

namespace purecpp::websearch {

/**
 * Implementação simples baseada em Gumbo-parser + html2text.
 * Converte HTML em Markdown “razoavelmente” limpo.
 *
 * ❶ Remove <script>, <style>, comentários.  
 * ❷ Usa Gumbo para extrair o corpo legível.  
 * ❸ Usa a pequena rotina html_to_md() para converter para MD.
 */
class HtmlCleaner : public ICleaner {
public:
    std::string to_markdown(const std::string& html,
                            const std::string& mime) override;
};

} // namespace purecpp::websearch
