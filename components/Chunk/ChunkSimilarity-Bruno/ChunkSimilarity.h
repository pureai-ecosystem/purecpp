#pragma once

#include <vector>
#include "CommonStructs.h"
#include "EmbeddingOpenAI.h"
#include "ChunkCommons/ChunkCommons.h"

namespace Chunk
{

    class ChunkSimilarity
    {

    public:
        ~ChunkSimilarity() = default;
        ChunkSimilarity(const int chunk_size = 500,
                        const int overlap = 200,
                        Chunk::EmbeddingModel embedding_model = Chunk::EmbeddingModel::OpenAI,
                        std::string model = "text-embedding-ada-002"
                    );
        std::vector<RAGLibrary::Document> Similarity(const std::vector<RAGLibrary::Document> &items= {}, int max_workers = 4);
        std::vector<RAGLibrary::Document> getChunksList() const;

    protected:

        std::vector<RAGLibrary::Document> Reorder(void);
        std::vector<RAGLibrary::Document> ProcessDocuments(const std::vector<RAGLibrary::Document> &items = {}, int max_workers = 4); 

    private:
        int m_chunk_size;
        int m_overlap;

        std::vector<RAGLibrary::Document> m_chunks_list;
        std::vector<std::vector<float>> m_chunk_embedding; // Vector of embeddings

        Chunk::EmbeddingModel m_embedding_model; 
        std::string m_model;
        std::string m_openai_api_key;

        void InitAPIKey();// Inicialização separada da chave
        std::vector<RAGLibrary::Document> Embeddings(const std::vector<RAGLibrary::Document>& list = {});

        void ValidateModel();// APAGAR
        std::vector<std::vector<float>> GenerateEmbeddings(const std::vector<std::string> &chunks); // APAGAR
    };

}