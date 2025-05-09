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
        
        std::vector<std::vector<float>> CreateVD(std::vector<RAGLibrary::Document> chunks_list = {}); // Geração de embeddings dos chunks
        std::vector<std::tuple<std::string, float, int>> Retrieve(float threshold = 0.5);// Recuperação de trechos similares
        std::string StrQ(int index = -1); // Resposta formatada
        std::vector<std::tuple<std::string, float, int>> getRetrieveList() const;
        RAGLibrary::Document getQuery() const;
        std::vector<RAGLibrary::Document> getChunksList() const; 
        std::pair<Chunk::EmbeddingModel, std::string> getPar() const;
        void printVD(int limit);
        
    protected:
        // Validações internas
        // bool queryCheck();
        // bool queryDocCheck();
        // bool checkChunkList();
        // bool checkEmbInChunks();
        // bool checkDims();
        bool allChunksHaveEmbeddings(const std::vector<RAGLibrary::Document>& chunks_list);

    private:
        // Atributos principais
        std::string m_query;
        std::vector<RAGLibrary::Document> m_chunks_list;
        RAGLibrary::Document m_query_doc;
        
        // Vetores de embeddings e trechos recuperados
        std::vector<float> m_emb_query;
        std::vector<std::vector<float>> m_chunk_embedding; // Vector of embeddings
        std::vector<std::tuple<std::string, float, int>> m_retrieve_list; // do I let as const or not?
        int quant_retrieve_list = 0;

        // Modelo de embedding (constante depois da construção)
        Chunk::EmbeddingModel m_embedding_model; 
        std::string m_model;
        
        std::string m_openai_api_key;// Chave da API do OpenAI

        bool initialized_ = false;// Allow only one instance of the chunks list to be created
        
        void InitAPIKey();// Inicialização separada da chave
        std::vector<RAGLibrary::Document> Embeddings(const std::vector<RAGLibrary::Document>& list);
    };

}
