#pragma once

#include "ChunkCommons/ChunkCommons.h"

#include <vector>
#include <string>

namespace Utils
{
    std::vector<std::string> GetLines(const std::string& filenamepath);
    std::string GetText(const std::string& filenamepath);
    Chunk::EmbeddingModel GetEmbeddingModel(const std::string& embedding_model);
}