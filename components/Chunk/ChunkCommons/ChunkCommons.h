#pragma once

#include "CommonStructs.h"

#include <re2/re2.h>
#include <torch/torch.h>
#include <vector>

namespace Chunk
{
    enum EmbeddingModel
    {
        HuggingFace,
        OpenAI
    };

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