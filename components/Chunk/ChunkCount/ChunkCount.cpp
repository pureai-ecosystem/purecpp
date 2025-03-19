#include "ChunkCommons/ChunkCommons.h"
#include "ChunkCount.h"
#include "RagException.h"
#include "StringUtils.h"

#include <omp.h>
#include <syncstream>

using namespace Chunk;

ChunkCount::ChunkCount(
    const std::string &count_unit,
    const int overlap,
    const int count_threshold)
    : m_count_unit(count_unit), m_overlap(overlap), m_count_threshold(count_threshold)
{
    ValidateCountUnit();
    std::string regex_ = "regex:";

    bool isRegex = m_count_unit.size() > regex_.size() &&
                   std::equal(regex_.begin(), regex_.end(), m_count_unit.begin());

    std::string pattern = m_count_unit;

    if (isRegex)
    {
        pattern = pattern.replace(0, regex_.size(), "");
    }
    else
    {
        pattern = StringUtils::escapeRegex(m_count_unit);
    }

    m_regex = std::make_shared<re2::RE2>("(" + pattern + ")");
}

void ChunkCount::ValidateCountUnit()
{
    if (m_count_unit.empty())
    {
        throw RAGLibrary::RagException("count_unit cannot be an empty string.");
    }
}

std::vector<RAGLibrary::Document> ChunkCount::ProcessSingleDocument(RAGLibrary::LoaderDataStruct &item)
{
    std::vector<RAGLibrary::Document> documents;
    try
    {
        auto chunks = Chunk::SplitTextByCount(item.textContent, m_overlap, m_count_threshold, m_regex);
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

std::vector<RAGLibrary::Document> ChunkCount::ProcessDocuments(const std::vector<RAGLibrary::LoaderDataStruct> &items, int max_workers)
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
            auto chunks = Chunk::SplitTextByCount(item.textContent, m_overlap, m_count_threshold, m_regex);

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