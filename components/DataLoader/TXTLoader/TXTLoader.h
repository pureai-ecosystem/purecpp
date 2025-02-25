#pragma once
#include <mutex>

#include "BaseLoader.h"

namespace TXTLoader
{
    class TXTLoader : public DataLoader::BaseDataLoader
    {
    public:
        TXTLoader() = delete;
        TXTLoader(const std::vector<RAGLibrary::DataExtractRequestStruct>& filePaths = {}, const unsigned int& numThreads = 0);
        ~TXTLoader() = default;

        void InsertDataToExtract(const std::vector<RAGLibrary::DataExtractRequestStruct>& dataPaths) final;
    private:
        void ExtractTextFromTXT(const RAGLibrary::DataExtractRequestStruct& path);
        std::string FileReader(const std::string& filePath);

        mutable std::mutex m_mutex;
    };
    using TXTLoaderPtr = std::shared_ptr<TXTLoader>;
}