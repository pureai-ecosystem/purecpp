#include "src/cleaner/HtmlCleaner.h"
#include <gumbo.h> // vcpkg: gumbo
#include <sstream>
#include <regex>

namespace purecpp::websearch {

/* ---------- helpers ------------------------------------------------- */

/* Remove tags <script>, <style> e comentários HTML. */
static std::string strip_noise(const std::string& html)
{
    static const std::regex scripts{R"(<(script|style)[\s\S]*?</\1>)",
                                    std::regex::icase};
    static const std::regex comments{R"()"};
    std::string out = std::regex_replace(html, scripts, "");
    out             = std::regex_replace(out, comments, "");
    return out;
}

/* Traverse Gumbo tree and collect visible text separated by \n\n */
static void walk(const GumboNode* node, std::ostringstream& oss)
{
    if (node->type == GUMBO_NODE_TEXT) {
        oss << node->v.text.text << ' ';
    } else if (node->type == GUMBO_NODE_ELEMENT) {
        const char* tag = gumbo_normalized_tagname(node->v.element.tag);
        if (tag && (std::string(tag) == "p" || std::string(tag) == "br"))
            oss << "\n";
        for (unsigned i = 0; i < node->v.element.children.length; ++i) {
            walk(static_cast<GumboNode*>(node->v.element.children.data[i]), oss);
        }
        if (tag && std::string(tag) == "p") oss << "\n";
    }
}

/* Extremely tiny HTML→Markdown (handles only #, links, lists, code) */
static std::string html_to_md(const std::string& txt)
{
    std::string out = txt;

    /* headings <h1-h6> */
    for (int h = 6; h >= 1; --h) {
        std::string open = "<h" + std::to_string(h) + ">";
        std::string clos = "</h" + std::to_string(h) + ">";
        std::regex rx{open + R"(([\s\S]*?))" + clos, std::regex::icase};
        out              = std::regex_replace(out, rx,
                             std::string(h, '#') + " $1\n");
    }

    /* links */
    {
        // CORREÇÃO: A inicialização da regex foi consolidada em uma única linha.
        std::regex rx{R"(<a [^>]*href="([^"]+)"[^>]*>([\s\S]*?)</a>)", std::regex::icase};
        out = std::regex_replace(out, rx, "[$2]($1)");
    }

    /* unordered list → “* item” */
    {
        std::regex li{R"(<li[^>]*>([\s\S]*?)</li>)", std::regex::icase};
        out = std::regex_replace(out, li, "* $1\n");
        std::regex ul{R"(<\/?ul[^>]*>)", std::regex::icase};
        out = std::regex_replace(out, ul, "");
    }

    /* code span */
    {
        std::regex code{R"(<code[^>]*>([\s\S]*?)</code>)",
                        std::regex::icase};
        out = std::regex_replace(out, code, "`$1`");
    }

    /* Strip remaining tags */
    out = std::regex_replace(out, std::regex{R"(<[^>]+>)"}, "");

    /* Collapse whitespace */
    out = std::regex_replace(out, std::regex(R"(\s+\n)"), "\n");
    out = std::regex_replace(out, std::regex(R"(\n{3,})"), "\n\n");

    return out;
}

/* ---------- public --------------------------------------------------- */

std::string HtmlCleaner::to_markdown(const std::string& html,
                                     const std::string& mime)
{
    if (mime.rfind("text/html", 0) != 0 && mime.rfind("application/xhtml", 0) != 0)
        return "```\n" + html + "\n```";   // fallback: não é HTML

    std::string clean = strip_noise(html);

    GumboOutput* output = gumbo_parse(clean.c_str());
    std::ostringstream oss;
    walk(output->root, oss);
    gumbo_destroy_output(&kGumboDefaultOptions, output);

    std::string extracted = oss.str();
    return html_to_md(extracted);
}

} // namespace purecpp::websearch