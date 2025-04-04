#pragma once

#include "IBaseEmbedding.h"

namespace Embedding
{
    class BaseEmbedding : public IBaseEmbedding
    {
    public:
        BaseEmbedding() = default;
        virtual ~BaseEmbedding() = default;

        virtual std::vector<float> GenerateEmbeddings(const std::vector<RAGLibrary::Document> &documents) = 0;
        virtual Document ProcessDocument(RAGLibrary::Document document) override;
        virtual std::vector<Document> ProcessDocuments(std::vector<RAGLibrary::Document> &documents, const int &maxWorkers) override;
    };
}