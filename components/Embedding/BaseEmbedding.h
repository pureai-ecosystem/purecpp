#pragma once

#include "IBaseEmbedding.h"

namespace Embedding
{
    class BaseEmbedding : public IBaseEmbedding
    {
    public:
        BaseEmbedding() = default;
        virtual ~BaseEmbedding() = default;

        virtual std::vector<float> GenerateEmbeddings(const std::vector<std::string>& text) = 0;
        virtual Document ProcessDocument(Document document) override;
        virtual std::vector<Document> ProcessDocuments(std::vector<Document> documents, const int& maxWorkers) override;
    };
}