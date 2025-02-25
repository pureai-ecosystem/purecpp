#include "Utils.h"

#include <fstream>

std::vector<std::string> Utils::GetLines(const std::string& filenamepath)
{
    std::ifstream file(filenamepath);

    std::vector<std::string> lines;
    if (file.is_open())
    {
        std::string line;
        while (std::getline(file, line))
        {
            lines.push_back(line);
        }

        file.close();
    }
    return lines;
}

std::string Utils::GetText(const std::string& filenamepath)
{
    std::ifstream file(filenamepath);

    std::string text;
    if (file.is_open())
    {
        std::string line;
        while (std::getline(file, line))
        {
            text.reserve(text.size() + line.size() + 1);
            if (!text.empty())
            {
                text.append("\n");
            }
            text.append(line.begin(), line.end());
        }

        file.close();
    }
    return text;
}

Chunk::EmbeddingModel Utils::GetEmbeddingModel(const std::string& embedding_model)
{
    Chunk::EmbeddingModel embeddingModel = Chunk::EmbeddingModel::HuggingFace;
    if (embedding_model == "hf") embeddingModel = Chunk::EmbeddingModel::HuggingFace;
    else if (embedding_model == "openai") embeddingModel = Chunk::EmbeddingModel::OpenAI;
    return embeddingModel;
}