#include "EmbeddingOpenAI.h"

#include <cstddef>
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

    std::vector<RAGLibrary::Document> EmbeddingOpenAI::GenerateEmbeddings(const std::vector<RAGLibrary::Document> &documents, const std::string &model, size_t batch_size = 32)
    {
        if (documents.empty())
            throw RAGLibrary::RagException("No documents provided for embedding.");

        if (model.empty())
            throw RAGLibrary::RagException("Model name cannot be empty.");

        std::vector<RAGLibrary::Document> processedDocuments = documents;
        const size_t total = processedDocuments.size();
        
        for(size_t i = 0; i < total; i+=batch_size)
        {
            const size_t end_idx = std::min(i + batch_size, total);
            std::vector<std::string> batch_texts;
            batch_texts.reserve(end_idx - i);

            for(size_t j = i; j < end_idx; j++)
            { 
                auto& doc = processedDocuments[j];
                if(doc.page_content.empty()){
                      throw RAGLibrary::RagException("Document content is empty at index: " + std::to_string(j));
                }
                batch_texts.push_back(doc.page_content);
            }

            auto response = openai::embedding().create({
                {"input", batch_texts},
                {"model", model}
            });
            if(response.contains("data") && response["data"].is_array() && !response["data"].empty())
            {
								auto& data = response["data"];
								
								if(data.size() != batch_texts.size())
								{
										throw RAGLibrary::RagException(
                    "Mismatch between batch size and response size. Expected: " +
                    std::to_string(batch_texts.size()) + " Received: " +
                    std::to_string(data.size()));
								}
								for (size_t b = 0; b < data.size(); ++b) 
								{
										const size_t doc_index = i + b;
										if (data[b].contains("embedding") && data[b]["embedding"].is_array()) {
												processedDocuments[doc_index].embedding = 
														data[b]["embedding"].get<std::vector<float>>();
										}
										else {
												throw RAGLibrary::RagException(
														"Malformed embedding response for document index: " + 
														std::to_string(doc_index));
										}
								}
            }
						else
						{
								throw RAGLibrary::RagException("API Error: " + response.dump() + "\nBatches indices: " + std::to_string(i) + "-" + std::to_string(end_idx - 1));
						}
        }          
        return processedDocuments;
    }
}
