#pragma once
#include <tuple>
#include <string>
#include <vector>
#include "CommonStructs.h"
#include "EmbeddingOpenAI.h"
#include "ChunkCommons.h"

namespace Chunk {

    class ChunkQuery {
    public:
        ChunkQuery(
            std::string query = "",
            RAGLibrary::Document query_doc = {},
            std::vector<RAGLibrary::Document> chunks_list = {},
            Chunk::EmbeddingModel embedding_model = Chunk::EmbeddingModel::OpenAI,
            std::string model = "text-embedding-ada-002"
        );
        ~ChunkQuery() = default;     
        RAGLibrary::Document Query(std::string query = "");
        RAGLibrary::Document Query(RAGLibrary::Document query_doc = {}); 
        
        std::vector<std::vector<float>> CreateVD(std::vector<RAGLibrary::Document> chunks_list = {});
        std::vector<std::tuple<std::string, float, int>> Retrieve(float threshold = 0.5);
        std::string StrQ(int index = -1);
        std::vector<std::tuple<std::string, float, int>> getRetrieveList() const;
        RAGLibrary::Document getQuery() const;
        std::vector<RAGLibrary::Document> getChunksList() const; 
        std::pair<Chunk::EmbeddingModel, std::string> getPair() const;
        void printVD(int limit);
        
    protected:
        bool allChunksHaveEmbeddings(const std::vector<RAGLibrary::Document>& chunks_list);

    private:
        std::string m_query;
        std::vector<RAGLibrary::Document> m_chunks_list;
        RAGLibrary::Document m_query_doc;
        
        std::vector<float> m_emb_query;
        std::vector<std::vector<float>> m_chunk_embedding;
        std::vector<std::tuple<std::string, float, int>> m_retrieve_list;
        int quant_retrieve_list = 0;

        Chunk::EmbeddingModel m_embedding_model; 
        std::string m_model;
        std::string m_openai_api_key;

        bool initialized_ = false;
        void InitAPIKey();
        std::vector<RAGLibrary::Document> Embeddings(const std::vector<RAGLibrary::Document>& list);
    };

}
