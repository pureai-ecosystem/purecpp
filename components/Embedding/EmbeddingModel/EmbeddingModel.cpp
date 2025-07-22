#include "EmbeddingModel.h"
#include "Chunk/ChunkCommons/ChunkCommons.h"
#include "RagException.h"

std::vector<RAGLibrary::Document> Embedding::EmbeddingModel::GenerateEmbeddings(const std::vector<RAGLibrary::Document> &documents, const std::string &model, size_t batch_size)
{
    std::vector<std::string> chunks_str;
    chunks_str.reserve(documents.size());
    for (const auto& doc : documents)
    {
        chunks_str.push_back(doc.page_content);
    }

    auto embeddings = Chunk::EmbeddingModelBatch(chunks_str, model, batch_size);

    if (embeddings.size() != documents.size())
    {
        throw RAGLibrary::RagException("Mismatch between number of documents and generated embeddings.");
    }

    std::vector<RAGLibrary::Document> result_docs = documents;
    for (size_t i = 0; i < documents.size(); ++i)
    {
        result_docs[i].embedding = embeddings[i];
    }

    return result_docs;
}
