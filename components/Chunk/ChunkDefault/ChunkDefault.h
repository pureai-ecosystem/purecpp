#pragma once

#include <regex>
#include <vector>
#include <re2/re2.h>
#include "ChunkCommons/ChunkCommons.h"
#include "CommonStructs.h"

namespace Chunk
{
    class ChunkDefault
    {
 
    public:
        ChunkDefault(const int chunk_size = 100, const int overlap = 20, std::optional<std::vector<RAGLibrary::Document>> items_opt = std::nullopt, int max_workers = 4);
        ~ChunkDefault() = default;
        //========================================================================================
        const std::vector<RAGLibrary::Document>& ProcessDocuments(std::optional<std::vector<RAGLibrary::Document>> items_opt = std::nullopt, int max_workers = 4);
        const Chunk::vdb_data& CreateEmb(std::string model = "text-embedding-ada-002"); 
        //========================================================================================
        void LogEmbeddingStats(std::string model, std::string vendor , size_t dim, size_t n, size_t flatVD_size) const;
        void printVD(void);
        //========================================================================================
        const std::vector<RAGLibrary::Document>& getChunks(void) const;
        const Chunk::vdb_data* getElement(size_t pos) const;
        //========================================================================================
        size_t quant_of_elements(void) const;
        //--------------------------------------------
        inline bool isInitialized(void) const{
            return initialized_;
        }
        //--------------------------------------------
        inline const std::vector<float>& getFlatVD(size_t i) const {
            if (i >= elements.size())
                throw std::out_of_range("Invalid index.");
            if (elements[i].flatVD.empty())
                throw std::runtime_error("flatVD is empty at index " + std::to_string(i));
            return elements[i].flatVD;
        }
        //--------------------------------------------
        void clear(void);
        
    private:
        std::map<std::string, std::string> metadata;
        std::vector<RAGLibrary::Document> chunks;
        std::vector<Chunk::vdb_data> elements;
        int m_chunk_size;
        int m_overlap;
        bool initialized_ = false;// Allow only one instance of the chunks list to be created
        
        std::vector<RAGLibrary::Document> ProcessSingleDocument(RAGLibrary::Document &item);
        inline bool is_this_model_used_yet(const std::string& modelo_procurado) {
            return std::any_of(
                this->elements.begin(), this->elements.end(),
                [&](const Chunk::vdb_data& vdb) {
                    return vdb.model == modelo_procurado;
                }
            );
        }


    };

}