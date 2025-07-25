#pragma once

#include "CommonStructs.h"

#include <re2/re2.h>
#include <torch/torch.h>
#include <vector>
#include <unordered_map>
#include <optional>
#include <algorithm>
#include <string>
#include <cctype>
#include "EmbeddingOpenAI.h"
namespace Chunk
{
    struct vdb_data {
        std::vector<float> flatVD;
        std::string vendor;
        std::string model;
        size_t dim = 0;
        size_t n = 0;
        //----------------------------------------------------
        inline const std::tuple<size_t, size_t>  getPar(void) const{return { n, dim };}; 
        inline std::pair<std::string, std::string>getEmbPar(void) const{return { vendor , model };}; 
        inline const float* getVDpointer(void) const{
            if (flatVD.empty()) {
                std::cout << "[Info] Empty Vector Data Base\n";
                return {};
            }
            return flatVD.data();
        }; 
    };
    
    //===============================================================================================    
        extern inline const std::unordered_map<std::string, std::vector<std::string>> EmbeddingModel = {
            {"openai", {"text-embedding-ada-002", "text-embedding-3-small", "..."}},
            {"huggingface", {"bge-small", "bge-large"}},
            {"cohere", {"embed-english-light-v3.0"}}
        };
 
        inline void PrintEmbeddingModels() {
        std::cout << "╔══════════════════════════════════════════════════════════╗\n";
        std::cout << "║               📦 Available Embedding Models              \n";
        std::cout << "╠══════════════════════════════════════════════════════════╣\n";

        for (const auto& [vendor, models] : EmbeddingModel) {
            std::cout << "║ 🔸 Vendor: " << vendor << "\n";
            for (const auto& model : models) {
                std::cout << "║    └── " << model << "\n";
            }
            std::cout << "╠──────────────────────────────────────────────────────────╣\n";
        }

        std::cout << "╚══════════════════════════════════════════════════════════╝\n";
    }


    inline std::string to_lowercase(std::string str) {
        std::transform(str.begin(), str.end(), str.begin(),
                    [](unsigned char c) { return std::tolower(c); });
        return str;
    }

    inline std::optional<std::string> resolve_vendor_from_model(const std::string& model) {
        for (const auto& [vendor, models] : EmbeddingModel) {
            if (std::find(models.begin(), models.end(), model) != models.end()) {
                return vendor;
            }
        }
        return std::nullopt;
    }

    inline bool resolve_vendor(const std::string& v) {
        for (const auto& [vendor, _] : EmbeddingModel) {
            if (vendor == v) return true;
        }
        return false;
    }
    
    inline size_t allChunksHaveEmbeddings(const std::vector<RAGLibrary::Document>& chunks_list) {
        if (chunks_list.empty()) return 0;

        const size_t ref_dim = chunks_list[0].embedding->size();

        // Valida todos os demais
        bool consistent = std::all_of(
            chunks_list.begin(),
            chunks_list.end(),
            [ref_dim](const RAGLibrary::Document& doc) {
                return doc.embedding.has_value() &&
                    doc.embedding->size() == ref_dim;
            });

        return consistent ? ref_dim : 0;
    }
    
    inline void InitAPIKey() {
        const char* env_key = std::getenv("OPENAI_API_KEY");
        if (env_key == nullptr) {
            #ifdef DEBUG
                std::cerr << "Erro: API key not set.\n";
            #endif
            throw std::runtime_error("API key not set. Please set the OPENAI_API_KEY environment variable.");
        }
        #ifdef DEBUG
        std::cerr << "API key set\n";
        #endif
    }
    std::vector<RAGLibrary::Document> Embeddings(const std::vector<RAGLibrary::Document>& list, std::string model);
   
    //===============================================================================================
    std::vector<float> MeanPooling(const std::vector<float> &token_embeddings, const std::vector<int64_t> &attention_mask, size_t embedding_size);
    void NormalizeEmbeddings(std::vector<float> &embeddings);

    std::vector<std::vector<float>> EmbeddingModelBatch(const std::vector<std::string> &chunks, const std::string &model, const int batch_size = 32);
    inline std::vector<std::vector<float>> EmbeddingHuggingFaceTransformers(const std::vector<std::string> &chunks)
    {
        return EmbeddingModelBatch(chunks, "sentence-transformers/all-MiniLM-L6-v2");
    }
    std::vector<std::vector<float>> EmbeddingOpeanAI(const std::vector<std::string> &chunks, const std::string &openai_api_key);

    at::Tensor toTensor(std::vector<std::vector<float>> &vect);

    std::vector<std::string> SplitText(std::string inputs, const int overlap, const int chunk_size);
    std::vector<std::string> SplitTextByCount(const std::string &input, int overlap, int count_threshold, const std::shared_ptr<re2::RE2> regex);
    
    // void InitAPIKey();// Inicialização separada da chave
    // std::vector<RAGLibrary::Document> Embeddings(const std::vector<RAGLibrary::Document>& list);
}
