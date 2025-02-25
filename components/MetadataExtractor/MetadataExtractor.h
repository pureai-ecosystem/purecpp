#pragma once

#include "IMetadataExtractor.h"

namespace MetadataExtractor
{
    class MetadataExtractor : public IMetadataExtractor
    {
    public:
        MetadataExtractor() = default;
        virtual ~MetadataExtractor() = default;

        virtual Document ProcessDocument(Document doc) = 0;
        virtual std::vector<Document> ProcessDocuments(std::vector<Document> docs, const int& maxWorkers) override;
    };
}