#pragma once

#include "MetadataExtractor.h"

namespace MetadataRegexExtractor
{
    class IMetadataRegexExtractor : public ::MetadataExtractor::MetadataExtractor
    {
    public:
        virtual ~IMetadataRegexExtractor() = default;

        virtual void AddPattern(const std::string& name, const std::string& pattern) = 0;
    };
    using IMetadataRegexExtractorPtr = std::shared_ptr<IMetadataRegexExtractor>;
}