#pragma once

#include "ChunkCommons/ChunkCommons.h"
#include "CommonStructs.h"

#include <vector>

namespace Chunk
{

    class ChunkSimilarity
    {

    public:
        ~ChunkSimilarity() = default;
        ChunkSimilarity(const int chunk_size = 100,
                        const int overlap = 20,
                        std::string embedding_model = "openai",
                        const std::string &openai_api_key = "");

        std::vector<RAGLibrary::Document> ProcessSingleDocument(const RAGLibrary::Document &item);
        std::vector<RAGLibrary::Document> ProcessDocuments(const std::vector<RAGLibrary::Document> &items, int max_workers = 4);

    protected:
        void ValidateModel();
        std::vector<std::vector<float>> GenerateEmbeddings(const std::vector<std::string> &chunks);

    private:
        int m_chunk_size;
        int m_overlap;
        std::string m_embedding_model;
        std::string m_openai_api_key;
    };

}
