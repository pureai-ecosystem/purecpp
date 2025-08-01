#ifndef METADATA_EXTRACTOR_H
#define METADATA_EXTRACTOR_H

#include <vector>

#include "Document.h"
#include "IMetadataExtractor.h"

namespace MetadataExtractor
{
    class MetadataExtractor : public IMetadataExtractor
    {
    public:
        MetadataExtractor() = default;
        virtual ~MetadataExtractor() = default;

        Document ProcessDocument(Document doc) override = 0;
        std::vector<Document> ProcessDocuments(std::vector<Document> docs, const int& maxWorkers) override;
    };
}
#endif
