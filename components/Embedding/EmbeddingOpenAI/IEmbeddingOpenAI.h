#pragma once

#include "BaseEmbedding.h"

namespace EmbeddingOpenAI
{
    class IEmbeddingOpenAI : public ::Embedding::BaseEmbedding
    {
    public:
       virtual ~IEmbeddingOpenAI() = default;

       virtual void SetAPIKey(const std::string& apiKey) = 0;
    };
    using IEmbeddingOpenAIPtr = std::shared_ptr<IEmbeddingOpenAI>;
}