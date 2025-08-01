#ifndef I_METADATA_EXTRACTOR_H
#define I_METADATA_EXTRACTOR_H

#include <memory>
#include <vector>

#include "Document.h"

namespace MetadataExtractor
{
    class IMetadataExtractor
    {
    public:
        virtual ~IMetadataExtractor() = default;

        virtual Document ProcessDocument(Document doc) = 0;
        virtual std::vector<Document> ProcessDocuments(std::vector<Document> docs, const int& maxWorkers) = 0;
    };
    using IMetadataExtractorPtr = std::shared_ptr<IMetadataExtractor>;
}
#endif
