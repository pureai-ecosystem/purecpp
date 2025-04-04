#pragma once

#include <memory>
#include <vector>
#include <string>

#include "CommonStructs.h"
#include "Document.h"

namespace Embedding
{
    class IBaseEmbedding
    {
    public:
        ~IBaseEmbedding() = default;

        virtual std::vector<float> GenerateEmbeddings(const std::vector<RAGLibrary::Document> &documents) = 0;
        virtual Document ProcessDocument(RAGLibrary::Document document) = 0;
        virtual std::vector<Document> ProcessDocuments(std::vector<RAGLibrary::Document> &documents, const int &maxWorkers) = 0;
    };
    using IBaseEmbeddingPtr = std::shared_ptr<IBaseEmbedding>;
}