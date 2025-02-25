#pragma once

#include <memory>
#include <vector>
#include <string>

#include "Document.h"

namespace Embedding
{
    class IBaseEmbedding
    {
    public:
        ~IBaseEmbedding() = default;

        virtual std::vector<float> GenerateEmbeddings(const std::vector<std::string>& text) = 0;
        virtual Document ProcessDocument(Document document) = 0;
        virtual std::vector<Document> ProcessDocuments(std::vector<Document> documents, const int& maxWorkers) = 0;
    };
    using IBaseEmbeddingPtr = std::shared_ptr<IBaseEmbedding>;
}