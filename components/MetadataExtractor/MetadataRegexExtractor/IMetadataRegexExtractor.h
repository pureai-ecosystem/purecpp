#ifndef I_METADATA_REGEX_EXTRACTOR_H
#define I_METADATA_REGEX_EXTRACTOR_H

#include <string>
#include <memory>


namespace MetadataRegexExtractor
{
    class IMetadataRegexExtractor
    {
    public:
        virtual ~IMetadataRegexExtractor() = default;

        virtual void AddPattern(const std::string& name, const std::string& pattern) = 0;
    };
    using IMetadataRegexExtractorPtr = std::shared_ptr<IMetadataRegexExtractor>;
}
#endif
