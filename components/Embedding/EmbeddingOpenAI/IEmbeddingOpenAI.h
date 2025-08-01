#ifndef I_EMBEDDING_OPENAI_H
#define I_EMBEDDING_OPENAI_H

#include "../IBaseEmbedding.h"

namespace EmbeddingOpenAI
{
    class IEmbeddingOpenAI : public ::Embedding::IBaseEmbedding
    {
    public:
       virtual ~IEmbeddingOpenAI() = default;

       virtual void SetAPIKey(const std::string& apiKey) = 0;
       // Note: GenerateEmbeddings is inherited from IBaseEmbedding and does not need to be redeclared here.
    };
    using IEmbeddingOpenAIPtr = std::shared_ptr<IEmbeddingOpenAI>;
}

#endif // I_EMBEDDING_OPENAI_H