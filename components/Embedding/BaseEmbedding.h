#pragma once

#include "IBaseEmbedding.h"

namespace Embedding
{
    class BaseEmbedding : public IBaseEmbedding
    {
    public:
        BaseEmbedding() = default;
        virtual ~BaseEmbedding() = default;

        virtual std::vector<RAGLibrary::Document> GenerateEmbeddings(const std::vector<RAGLibrary::Document> &documents) = 0;
    };
}