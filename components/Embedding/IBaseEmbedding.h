#pragma once

#include <memory>
#include <vector>
#include <string>

#include "CommonStructs.h"
#include "Document.h"

namespace Embedding
{
    class IBaseEmbedding
    {
    public:
        ~IBaseEmbedding() = default;

        virtual std::vector<RAGLibrary::Document> GenerateEmbeddings(const std::vector<RAGLibrary::Document> &documents, const std::string &model) = 0;
    };
    using IBaseEmbeddingPtr = std::shared_ptr<IBaseEmbedding>;
}