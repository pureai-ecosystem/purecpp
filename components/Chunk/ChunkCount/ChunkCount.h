#pragma once

#include "CommonStructs.h"

#include <re2/re2.h>
#include <vector>

namespace Chunk
{
    class ChunkCount
    {

    public:
        ChunkCount() = default;
        ChunkCount(const std::string &count_unit, const int overlap = 600, const int count_threshold = 1);
        ~ChunkCount() = default;

        std::vector<RAGLibrary::Document> ProcessSingleDocument(RAGLibrary::LoaderDataStruct &item);
        std::vector<RAGLibrary::Document> ProcessDocuments(const std::vector<RAGLibrary::LoaderDataStruct> &items, int max_workers = 4);

    protected:
        void ValidateCountUnit();
        std::vector<std::string> SplitByCount(const std::vector<std::string> &texts);

    private:
        std::string m_count_unit;
        int m_overlap;
        int m_count_threshold;
        std::shared_ptr<re2::RE2> m_regex;
    };

}