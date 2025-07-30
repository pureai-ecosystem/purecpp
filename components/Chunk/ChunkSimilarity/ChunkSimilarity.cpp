#include "ChunkCommons/ChunkCommons.h"
#include "ChunkSimilarity.h"
#include "RagException.h"
#include "StringUtils.h"

#include <cmath>
#include <fstream>
#include <omp.h>
#include <syncstream>

using namespace Chunk;

ChunkSimilarity::ChunkSimilarity(
    const int chunk_size,
    const int overlap,
    std::string embedding_model,
    const std::string &openai_api_key)
    : m_chunk_size(chunk_size), m_overlap(overlap), m_embedding_model(to_lowercase(embedding_model))
{
    ValidateModel();
}

void ChunkSimilarity::ValidateModel()
{
    if (m_overlap >= m_chunk_size)
    {
        throw RAGLibrary::RagException("The overlap value must be smaller than the chunk size.");
    }

    if(!Chunk::resolve_vendor(this->m_embedding_model)) 
        throw RAGLibrary::RagException("Invalid model.");

}

std::vector<std::vector<float>> ChunkSimilarity::GenerateEmbeddings(const std::vector<std::string> &chunks)
{
    std::vector<std::vector<float>> results;
    if(this->m_embedding_model=="huggingface")
        results = Chunk::EmbeddingHuggingFaceTransformers(chunks);
    else if(this->m_embedding_model=="openai")
        results = Chunk::EmbeddingOpeanAI(chunks, m_openai_api_key);
    return results;
}

std::vector<RAGLibrary::Document> ChunkSimilarity::ProcessSingleDocument(const RAGLibrary::Document &item)
{
    std::vector<RAGLibrary::Document> documents;
    try
    {
        auto chunks = Chunk::SplitText(item.page_content, m_overlap, m_chunk_size);
        auto embeddings = GenerateEmbeddings(chunks);
        auto embeddingsTensor = Chunk::toTensor(embeddings);
        auto similarity_matrix = torch::inner(embeddingsTensor, embeddingsTensor);
        auto sorted_indices = torch::argsort(-similarity_matrix.sum(1));

        documents.reserve(documents.size() + chunks.size());
        for (int i = 0; i < sorted_indices.size(0); i++)
        {
            auto j = sorted_indices[i].item<int64_t>();
            documents.push_back(RAGLibrary::Document(item.metadata, chunks[j]));
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        throw;
    }

    return documents;
}

std::vector<RAGLibrary::Document> ChunkSimilarity::ProcessDocuments(const std::vector<RAGLibrary::Document> &items, int max_workers)
{
    std::vector<RAGLibrary::Document> documents;
    try
    {
        int max_threads = omp_get_max_threads();
        if (max_workers > 0 && max_workers < max_threads)
        {
            max_threads = max_workers;
        }

        omp_set_num_threads(max_threads);
#pragma omp parallel for
        for (int i = 0; i < items.size(); i++)
        {
            auto &item = items[i];
            auto docs = ProcessSingleDocument(item);
#pragma omp critical
            {
                documents.reserve(documents.size() + docs.size());
                documents.insert(documents.end(), docs.begin(), docs.end());
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        throw;
    }

    return documents;
}
