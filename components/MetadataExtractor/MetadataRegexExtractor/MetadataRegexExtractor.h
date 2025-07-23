#pragma once

#include "IMetadataRegexExtractor.h"

#include "Document.h"
#include "re2/re2.h"

namespace MetadataRegexExtractor
{
    using RE2Ptr = std::shared_ptr<RE2>;
    class MetadataRegexExtractor : public IMetadataRegexExtractor
    {
    public:
        MetadataRegexExtractor();
        ~MetadataRegexExtractor() = default;

        void AddPattern(const std::string& name, const std::string& pattern) final;
        ::MetadataExtractor::Document ProcessDocument(::MetadataExtractor::Document doc) final;

    private:
        std::map<std::string, RE2Ptr> m_patterns;
    };
}