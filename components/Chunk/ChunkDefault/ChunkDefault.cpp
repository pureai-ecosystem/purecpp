#include "ChunkCommons/ChunkCommons.h"
#include "ChunkDefault.h"
#include "RagException.h"
#include "StringUtils.h"

#include <cmath>

#include <omp.h>
#include <syncstream>

using namespace Chunk;

ChunkDefault::ChunkDefault(
    const int chunk_size,
    const int overlap)
    : m_chunk_size(chunk_size), m_overlap(overlap)
{
    ValidateChunkSizeOverlap();
}

void ChunkDefault::ValidateChunkSizeOverlap()
{
    if (m_overlap >= m_chunk_size)
    {
        throw RAGLibrary::RagException("The overlap value must be smaller than the chunk size.");
    }
}

std::vector<RAGLibrary::Document> ChunkDefault::ProcessSingleDocument(RAGLibrary::LoaderDataStruct &item)
{
    std::vector<RAGLibrary::Document> documents;
    try
    {
        auto chunks = Chunk::SplitText(item.textContent, m_overlap, m_chunk_size);
        documents.reserve(documents.size() + chunks.size());
        for (auto &chunk : chunks)
        {
            documents.push_back(RAGLibrary::Document(item.metadata, chunk));
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        throw;
    }

    return documents;
}

std::vector<RAGLibrary::Document> ChunkDefault::ProcessDocuments(const std::vector<RAGLibrary::LoaderDataStruct> &items, int max_workers)
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
            auto chunks = Chunk::SplitText(item.textContent, m_overlap, m_chunk_size);

            #pragma omp critical
            {
                documents.reserve(documents.size() + chunks.size());
                for (auto &chunk : chunks)
                {
                    documents.push_back(RAGLibrary::Document(item.metadata, chunk));
                }
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