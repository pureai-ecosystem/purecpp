#pragma once
#include <tuple>
#include <string>
#include <vector>
#include "CommonStructs.h"
#include "ChunkCommons/ChunkCommons.h"
#include "ChunkDefault/ChunkDefault.h"

namespace Chunk {

    class ChunkQuery {
    public:
        ChunkQuery(
            std::string query = "",
            RAGLibrary::Document query_doc = {},
            const Chunk::ChunkDefault* chunks = nullptr,
            std::optional<size_t> pos = std::nullopt,
            float threshold = -5
        );
        ~ChunkQuery() = default;     
        // ======================================================================================================================================
        std::vector<std::tuple<std::string, float, int>> Retrieve(float threshold = 0.5, const Chunk::ChunkDefault* temp_chunks= nullptr, std::optional<size_t> pos = std::nullopt);  
        // ======================================================================================================================================
        RAGLibrary::Document Query(RAGLibrary::Document query_doc = {}, const Chunk::ChunkDefault* temp_chunks = nullptr, std::optional<size_t> pos = std::nullopt); 
        RAGLibrary::Document Query(std::string query = "", const Chunk::ChunkDefault* temp_chunks = nullptr, std::optional<size_t> pos = std::nullopt);
        // ======================================================================================================================================
        std::vector<std::tuple<std::string, float, int>> getRetrieveList(void) const;
        const std::vector<RAGLibrary::Document>& getChunksList(void) const; 
        std::tuple<size_t, size_t, size_t> getPar(void) const;
        inline const Chunk::vdb_data* getVDB() const {
            return m_vdb;
        }
        RAGLibrary::Document getQuery(void) const;
        std::string getMod(void) const;
        void setChunks(const Chunk::ChunkDefault& chunks, size_t pos);
        //-------------------------------------------------------------
        std::vector<float> getEmbedQuery(void) const;
        // ======================================================================================================================================
        std::string StrQ(int index = -1); // Resposta formatada
        // ======================================================================================================================================
    private:
        // Atributos pertensentes apenas ao query
        RAGLibrary::Document m_query_doc;
        std::vector<float> m_emb_query;
        std::string m_query;

        size_t m_n_chunk = 0;
        size_t m_pos = 0;
        size_t m_dim = 0;
        size_t m_n = 0;

        // trechos recuperados
        std::vector<std::tuple<std::string, float, int>> m_retrieve_list; 
        size_t quant_retrieve_list = 0;

        // others
        const std::vector<RAGLibrary::Document>* m_chunks_list = nullptr;
        const Chunk::ChunkDefault* m_chunks = nullptr;
        const Chunk::vdb_data* m_vdb = nullptr;
        
        // =====================================================================================================================================
        std::vector<std::span<const float>> m_chunk_embedding;    
        // =====================================================================================================================================
        inline RAGLibrary::Document validateEmbeddingResult(const std::vector<RAGLibrary::Document>& results) {
            if (results.empty() || !results[0].embedding.has_value()) {
                throw std::runtime_error("Embedding not present in result.");
            }
            return results[0];
        }
    };

}


