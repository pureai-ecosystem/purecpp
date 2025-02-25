#include "EmbeddingOpenAI.h"

#include <iostream>
#include "RagException.h"
#include "openai/openai.hpp"

namespace EmbeddingOpenAI
{
    void EmbeddingOpenAI::SetAPIKey(const std::string& apiKey)
    {
        m_ApiKey = apiKey;
        openai::start(m_ApiKey);
    }

    std::vector<float> EmbeddingOpenAI::GenerateEmbeddings(const std::vector<std::string>& text)
    {
        std::string embeddingString;
        for(auto& elem : text)
        {
            embeddingString += elem + " ";
        }

        auto res = openai::embedding().create({
            {"model", "text-embedding-ada-002"},
            {"input", embeddingString}
        });

        if(res.is_null()) 
        {
            throw RAGLibrary::RagException("JSON Response is null.");
        }
        
        return res["data"][0]["embedding"];
    }
}