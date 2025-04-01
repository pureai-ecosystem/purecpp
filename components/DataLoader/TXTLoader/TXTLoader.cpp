#include "TXTLoader.h"

#include <fstream>
#include <syncstream>
#include <filesystem>

#include <boost/algorithm/string.hpp>

#include "FileUtilsLocal.h"

namespace TXTLoader
{
    TXTLoader::TXTLoader(const std::string filePath, const unsigned int &numThreads) : DataLoader::BaseDataLoader(numThreads)
    {
        AddThreadsCallback([this](RAGLibrary::DataExtractRequestStruct value)
                           { ExtractTextFromTXT(value); });

        if (!filePath.empty())
        {
            LocalFileReader(filePath, ".txt");
        }
    }

    void TXTLoader::ExtractTextFromTXT(const RAGLibrary::DataExtractRequestStruct &path)
    {
        auto txtContent = RAGLibrary::FileReader(path.targetIdentifier);

        if (!std::any_of(txtContent.begin(), txtContent.end(), ::isgraph))
            return;

        std::lock_guard lock(m_mutex);
        std::filesystem::path filePath(path.targetIdentifier);
        RAGLibrary::Metadata metadata = {{"source", filePath.string()}};
        m_dataVector.emplace_back(metadata, txtContent);
    }
}