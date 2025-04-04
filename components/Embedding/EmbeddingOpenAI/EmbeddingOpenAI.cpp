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

    std::vector<float> EmbeddingOpenAI::GenerateEmbeddings(const std::vector<RAGLibrary::Document> &documents)
    {
        for (auto &doc : documents)
        {
            auto text = doc.page_content;
            if (text.empty())
                throw RAGLibrary::RagException("Document content is empty.");

            auto values = openai::embedding().create(openai::_detail::Json{
                {"input", std::vector<std::string>{text}},
                {"model", "text-embedding-ada-002"},
            })["data"][0]["embedding"];

            if (values.is_array())
            {
                doc.embedding = values.get<std::vector<float>>();
            }
        }
    }
}