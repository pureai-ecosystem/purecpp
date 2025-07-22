#pragma once

#include "IEmbeddingOpenAI.h"

namespace EmbeddingOpenAI
{
    const std::string APIKeyOpenAI = "";
    class EmbeddingOpenAI : public IEmbeddingOpenAI
    {
    public:
        EmbeddingOpenAI() = default;
        virtual ~EmbeddingOpenAI() = default;

        void SetAPIKey(const std::string &apiKey) final;
        std::vector<RAGLibrary::Document> GenerateEmbeddings(const std::vector<RAGLibrary::Document> &documents, const std::string &model, size_t batch_size = 32) final;

    private:
        std::string m_ApiKey;
        std::string m_modelName;
    };
}
