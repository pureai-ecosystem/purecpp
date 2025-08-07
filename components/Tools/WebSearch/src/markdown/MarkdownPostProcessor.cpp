#include "src/markdown/MarkdownPostProcessor.h"
#include <regex>
#include <sstream>

namespace purecpp::websearch::md {

/* ---------------- helpers ---------------- */

static inline std::string rtrim(const std::string& s)
{
    size_t end = s.find_last_not_of(" \t\r");
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

/* ---------------- run() ------------------ */

std::string& MarkdownPostProcessor::run(std::string& md) const
{
    /* 1. remove duplos espaços (fora de bloco código) */
    {
        std::regex dbl_space{R"(  +)"};
        md = std::regex_replace(md, dbl_space, " ");
    }

    /* 2. colapsa múltiplos \n para no máx 2 */
    {
        std::regex many_nl{R"(\n{3,})"};
        md = std::regex_replace(md, many_nl, "\n\n");
    }

    /* 3. trim em cada linha */
    std::ostringstream cleaned;
    std::istringstream iss(md);
    std::string line;
    while (std::getline(iss, line)) {
        cleaned << rtrim(line) << '\n';
    }
    md = cleaned.str();

    /* 4. força newline final */
    if (!md.empty() && md.back() != '\n') md.push_back('\n');

    return md;
}

} // namespace purecpp::websearch::md
