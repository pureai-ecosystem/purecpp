#pragma once

#include "Embedding/IBaseEmbedding.h"

namespace Embedding {

    class EmbeddingModel : public IBaseEmbedding
    {
    public:
        EmbeddingModel() = default;
        virtual ~EmbeddingModel() = default;

        std::vector<RAGLibrary::Document> GenerateEmbeddings(const std::vector<RAGLibrary::Document> &documents, const std::string &model, size_t batch_size = 32) override;
    };

}
