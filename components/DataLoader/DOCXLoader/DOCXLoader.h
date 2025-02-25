#pragma once

#include <filesystem>
#include <string_view>
#include <utility>

#include "BaseLoader.h"
namespace DOCXLoader
{
    constexpr std::string_view xmlDefaultPath = "word/document.xml";

    class DOCXLoader : public DataLoader::BaseDataLoader
    {
    public:
        DOCXLoader() = default;
        DOCXLoader(const std::vector<RAGLibrary::DataExtractRequestStruct> &filePaths = {}, const unsigned int &numThreads = 0);
        ~DOCXLoader() = default;

        void InsertDataToExtract(const std::vector<RAGLibrary::DataExtractRequestStruct> &dataPaths) final;

    private:
        std::optional<std::pair<std::string, int>> ExtractZIPFile(const RAGLibrary::DataExtractRequestStruct &path);
        void ExtractTextFromXML(std::filesystem::path filePath, const std::pair<std::string, int> &data);

        mutable std::mutex m_mutex;
    };
    using DOCXLoaderPtr = std::shared_ptr<DOCXLoader>;
}