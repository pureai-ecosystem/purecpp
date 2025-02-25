#include "ChunkCommons/ChunkCommons.h"
#include "ChunkQuery.h"
#include "RagException.h"
#include "StringUtils.h"

#include <cmath>
#include <fstream>
#include <omp.h>
#include <syncstream>

using namespace Chunk;

ChunkQuery::ChunkQuery(
    const int chunk_size,
    const int overlap,
    const EmbeddingModel embedding_model,
    const std::string &openai_api_key)
    : m_chunk_size(chunk_size), m_overlap(overlap), m_embedding_model(embedding_model)
{
    ValidateModel();
}

void ChunkQuery::ValidateModel()
{
    if (m_overlap >= m_chunk_size)
    {
        throw RAGLibrary::RagException("The overlap value must be smaller than the chunk size.");
    }

    switch (m_embedding_model)
    {
    case HuggingFace:
        break;
    case OpenAI:
        const auto open_api_key = std::getenv("OPENAI_API_KEY");
        if (m_openai_api_key.empty() && (!open_api_key || std::strcmp("", open_api_key) == 0))
        {
            throw RAGLibrary::RagException("The OpenAI API key is required to use the 'openai' model.");
        }
        break;
    }
}

std::vector<std::vector<float>> ChunkQuery::GenerateEmbeddings(const std::vector<std::string> &chunks)
{
    std::vector<std::vector<float>> results;
    switch (this->m_embedding_model)
    {
    case EmbeddingModel::HuggingFace:
        results = Chunk::EmbeddingHuggingFaceTransformers(chunks);
        break;
    case EmbeddingModel::OpenAI:
        results = Chunk::EmbeddingOpeanAI(chunks, m_openai_api_key);
        break;
    }

    return results;
}

std::vector<RAGLibrary::Document> ChunkQuery::ProcessSingleDocument(const RAGLibrary::LoaderDataStruct &item,
                                                                    const std::vector<float> &query_embedding,
                                                                    const float similarity_threshold)
{
    std::vector<RAGLibrary::Document> documents;
    try
    {
        auto chunks = Chunk::SplitText(item.textContent, m_overlap, m_chunk_size);
        auto chunksEmbeddings = GenerateEmbeddings(chunks);
        auto queryEmbeddingTensor = torch::from_blob(const_cast<float *>(query_embedding.data()), {int64_t(query_embedding.size())}, torch::kFloat32);
        for (size_t i = 0; i < chunks.size(); i++)
        {
            auto &chunk = chunks[i];
            auto &embedding = chunksEmbeddings[i];
            auto embeddingTensor = torch::from_blob(embedding.data(), {int64_t(embedding.size())}, torch::kFloat32);

            auto similarityTensor = torch::dot(queryEmbeddingTensor, embeddingTensor) / (torch::norm(queryEmbeddingTensor) * torch::norm(embeddingTensor));

            if (similarityTensor.item<float>() >= similarity_threshold)
            {
                documents.push_back(RAGLibrary::Document(item.metadata, chunks[i]));
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

std::vector<RAGLibrary::Document> ChunkQuery::ProcessDocuments(const std::vector<RAGLibrary::LoaderDataStruct> &items,
                                                               const std::string &query,
                                                               const float similarity_threshold,
                                                               int max_workers)
{
    std::vector<RAGLibrary::Document> documents;
    try
    {
        int max_threads = omp_get_max_threads();
        if (max_workers > 0 && max_workers < max_threads)
        {
            max_threads = max_workers;
        }

        auto query_embedding = GenerateEmbeddings({query}).front();

        omp_set_num_threads(max_threads);
        #pragma omp parallel for
        for (size_t i = 0; i < items.size(); i++)
        {
            auto &item = items[i];
            auto docs = ProcessSingleDocument(item, query_embedding, similarity_threshold);
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