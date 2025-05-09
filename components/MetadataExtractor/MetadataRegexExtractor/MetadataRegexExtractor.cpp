#include "MetadataRegexExtractor.h"

#include <algorithm>
#include <future>
#include <atomic>
#include <semaphore>
// #if __has_include(<semaphore>)
//     #include <semaphore>
// #else
//     // Alternativa: inclua uma implementação própria ou de terceiros
//     #include "semaphore_compat.h"
// #endif

#include <mutex>

namespace MetadataRegexExtractor
{
    MetadataRegexExtractor::MetadataRegexExtractor()
    {
        m_patterns["ProperName"] = std::make_shared<RE2>("[A-Z][a-z]+(?:\\s[A-Z][a-z]+)*");
        m_patterns["Date"] = std::make_shared<RE2>("\\b\\d{1,2}/\\d{1,2}/\\d{2,4}\\b|\\b\\d{4}-\\d{2}-\\d{2}\\b");
        m_patterns["Number"] = std::make_shared<RE2>("\\b\\d+\\b");
        m_patterns["Email"] = std::make_shared<RE2>("\\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}\\b");
        m_patterns["URL"] = std::make_shared<RE2>("\\bhttps?://[^\\s]+\\b");
    }

    void MetadataRegexExtractor::AddPattern(const std::string& name, const std::string& pattern)
    {
        m_patterns[name] = std::make_shared<RE2>(pattern);
    }

    ::MetadataExtractor::Document MetadataRegexExtractor::ProcessDocument(::MetadataExtractor::Document doc)
    {
        for(auto index = 0; index < doc.pageContent.size(); ++index)
        {
            std::for_each(m_patterns.begin(), m_patterns.end(), [this, &doc, index](std::pair<std::string, RE2Ptr> pair){
                if(RE2::FullMatch(doc.pageContent[index], *pair.second))
                {
                    doc.metadata.emplace_back(doc.pageContent[index], pair.first);
                }
            });
        }
        return doc;
    }
    

}