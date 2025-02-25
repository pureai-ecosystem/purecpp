#pragma once

#include <regex>
#include <vector>
#include <re2/re2.h>

#include "CommonStructs.h"

namespace Chunk
{
    class ChunkDefault
    {

    public:
        ChunkDefault(const int chunk_size = 100, const int overlap = 20);
        ~ChunkDefault() = default;

        std::vector<RAGLibrary::Document> ProcessSingleDocument(RAGLibrary::LoaderDataStruct &item);
        std::vector<RAGLibrary::Document> ProcessDocuments(const std::vector<RAGLibrary::LoaderDataStruct> &items, int max_workers = 4);

    protected:
        void ValidateChunkSizeOverlap();

    private:
        int m_chunk_size;
        int m_overlap;
    };

}