#include "EmbeddingOpenAI.h"

#include <iostream>
#include "RagException.h"
#include "openai/openai.hpp"

namespace EmbeddingOpenAI
{
    void EmbeddingOpenAI::SetAPIKey(const std::string &apiKey)
    {
        m_ApiKey = apiKey;
        openai::start(m_ApiKey);
    }

    std::vector<RAGLibrary::Document> EmbeddingOpenAI::GenerateEmbeddings(const std::vector<RAGLibrary::Document> &documents)
    {
        std::vector<RAGLibrary::Document> processedDocuments = documents;

        for (auto &doc : processedDocuments)
        {
            auto text = doc.page_content;
            if (text.empty())
                throw RAGLibrary::RagException("Document content is empty.");

            auto response = openai::embedding().create(openai::_detail::Json{
                {"input", std::vector<std::string>{text}},
                {"model", "text-embedding-ada-002"},
            });

            if (response.contains("data") && response["data"].is_array() && !response["data"].empty())
            {
                auto values = response["data"][0]["embedding"];
                if (values.is_array())
                {
                    doc.embedding = values.get<std::vector<float>>();
                }
            }
            else
            {
                throw RAGLibrary::RagException("Failed to generate embeddings.");
            }
        }

        return processedDocuments;
    }
}

