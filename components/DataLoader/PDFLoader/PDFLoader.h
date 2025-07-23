#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <map>

#include "BaseLoader.h"

namespace PDFLoader 
{

    class PDFLoader : public DataLoader::BaseDataLoader
    {
    public:
        PDFLoader() = delete;
        PDFLoader(const std::string filePath, const unsigned int &numThreads = 1);
        ~PDFLoader() = default;

        void InsertDataToExtract(const std::vector<RAGLibrary::DataExtractRequestStruct>& dataPaths);
    private:
        void ExtractPDFData(const RAGLibrary::DataExtractRequestStruct& path);
        
        mutable std::mutex m_mutex;
        std::condition_variable m_condVar;
    };
    using PDFLoaderPtr = std::shared_ptr<PDFLoader>; 
}