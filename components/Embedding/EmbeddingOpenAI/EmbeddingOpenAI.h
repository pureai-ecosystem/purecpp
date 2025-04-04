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
        std::vector<float> GenerateEmbeddings(const std::vector<RAGLibrary::Document> &documents) final;

    private:
        std::string m_ApiKey;
        std::string m_modelName;
    };
}