#pragma once

#include "BaseEmbedding.h"

namespace EmbeddingOpenAI
{
    class IEmbeddingOpenAI : public ::Embedding::BaseEmbedding
    {
    public:
       virtual ~IEmbeddingOpenAI() = default;

              virtual void SetAPIKey(const std::string& apiKey) = 0;
       virtual std::vector<RAGLibrary::Document> GenerateEmbeddings(const std::vector<RAGLibrary::Document> &documents, const std::string &model, size_t batch_size) = 0;
    };
    using IEmbeddingOpenAIPtr = std::shared_ptr<IEmbeddingOpenAI>;
}