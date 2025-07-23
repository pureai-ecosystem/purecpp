#pragma once

#include "Document.h"
#include "MetadataExtractor.h"

#include <map>
#include <string>

namespace MetadataHFExtractor
{
    class IMetadataHFExtractor : public ::MetadataExtractor::MetadataExtractor
    {
    public:
        virtual ~IMetadataHFExtractor() = default;

        virtual void InitializeNERModel() = 0;

        virtual std::vector<std::pair<std::string, std::string>> ExtractMetadata(const std::vector<std::string>& text) = 0;
    };
    using IMetadataHFExtractorPtr = std::shared_ptr<IMetadataHFExtractor>;
}