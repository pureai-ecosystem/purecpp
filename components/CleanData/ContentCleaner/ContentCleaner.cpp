#include <omp.h>
#include <re2/re2.h>
#include <format>
#include <memory>

#include "RagException.h"
#include "ContentCleaner.h"

using namespace CleanData;
#include <string_view>
constexpr std::string_view extraSpaces = "\\s+";
constexpr std::string_view nonAsciiCharacters = "[^\\x00-\\x7F]+";
constexpr std::string_view symbolsBeginningOrEndOfLines = "^\\W+|\\W+$";

static void trim(std::string& text)
{
    auto isNoSpace = [](unsigned char ch)
    {
        return !std::isspace(ch);
    };
    // remove whitespace from the beginning of the string
    text.erase(text.begin(), std::find_if(text.begin(), text.end(), isNoSpace));
    // remove whitespace from the end of the string
    text.erase(std::find_if(text.rbegin(), text.rend(), isNoSpace).base(), text.end());
}

ContentCleaner::ContentCleaner(const std::vector<std::string>& default_patterns)
: m_default_patterns(default_patterns)
{
    if (m_default_patterns.empty())
    {
       m_default_patterns = std::vector<std::string>{
            std::string(extraSpaces),
            std::string(nonAsciiCharacters),
            std::string(symbolsBeginningOrEndOfLines)
        };
    }
    ValidatePatterns(m_default_patterns);
}

void ContentCleaner::ValidatePatterns(const std::vector<std::string>& patterns)
{
    for(auto& pattern : patterns)
    {
        re2::RE2 re2(pattern);
        if(!re2.ok())
        {
            throw RAGLibrary::RagException(std::format("IsRegularPattern: {} error: {}", pattern.c_str(), re2.error().c_str()));
       }
    }
}

std::string ContentCleaner::CleanContent(const std::string& text, const std::vector<std::string>& custom_patterns)
{
    ValidatePatterns(custom_patterns);
    std::vector<std::shared_ptr<re2::RE2>> regexs;
    regexs.reserve(m_default_patterns.size() + custom_patterns.size());

    auto appendRegex = [&regexs](const std::string& pattern)
    {
        regexs.push_back(std::make_shared<re2::RE2>(pattern));
    };

    std::for_each(m_default_patterns.begin(), m_default_patterns.end(), appendRegex);
    std::for_each(custom_patterns.begin(), custom_patterns.end(), appendRegex);

    std::string replacedText = text;

    for(auto& regex : regexs)
    {
        re2::RE2::GlobalReplace(&replacedText, *regex, " ");
    }

    trim(replacedText);

    return replacedText;
}

RAGLibrary::Document ContentCleaner::ProcessDocument(const RAGLibrary::Document& doc, const std::vector<std::string>& custom_patterns)
{
    try
    {
        return RAGLibrary::Document(doc.metadata, CleanContent(doc.page_content, custom_patterns));
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        throw;
    }
}

std::vector<RAGLibrary::Document> ContentCleaner::ProcessDocuments(const std::vector<RAGLibrary::Document>& docs, const std::vector<std::string>& custom_patterns, int max_workers)
{
    std::vector<RAGLibrary::Document> documents(docs.size());
    int max_threads = omp_get_max_threads();
    if (max_workers > 0 && max_workers < max_threads)
    {
        max_threads = max_workers;
    }

    omp_set_num_threads(max_threads);
    #pragma omp parallel for
    for (size_t i = 0; i < docs.size(); i++)
    {
        documents[i] = ProcessDocument(docs[i], custom_patterns);
    }

    return documents;
}
