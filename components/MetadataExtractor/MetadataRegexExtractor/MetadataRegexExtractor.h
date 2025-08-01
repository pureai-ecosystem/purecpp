#ifndef METADATA_REGEX_EXTRACTOR_H
#define METADATA_REGEX_EXTRACTOR_H

#include <map>
#include <memory>

#include "../MetadataExtractor.h"
#include "IMetadataRegexExtractor.h"

#include "../Document.h"
#include "re2/re2.h"

namespace MetadataRegexExtractor
{
    using RE2Ptr = std::shared_ptr<RE2>;
    class MetadataRegexExtractor : public ::MetadataExtractor::MetadataExtractor, public IMetadataRegexExtractor
    {
    public:
        MetadataRegexExtractor();
        ~MetadataRegexExtractor() override = default;

        void AddPattern(const std::string& name, const std::string& pattern) override;
        ::MetadataExtractor::Document ProcessDocument(::MetadataExtractor::Document doc) override;

    private:
        std::map<std::string, RE2Ptr> m_patterns;
    };
}
#endif
