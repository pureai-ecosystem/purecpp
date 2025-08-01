#ifndef TXT_LOADER_H
#define TXT_LOADER_H
#include <mutex>

#include "BaseLoader.h"

namespace TXTLoader
{
    class TXTLoader : public DataLoader::BaseDataLoader
    {
    public:
        TXTLoader() = delete;
        TXTLoader(const std::string filePath, const unsigned int &numThreads = 1);
        ~TXTLoader() = default;

    private:
        void ExtractTextFromTXT(const RAGLibrary::DataExtractRequestStruct &path);
        std::string FileReader(const std::string &filePath);

        mutable std::mutex m_mutex;
    };
    using TXTLoaderPtr = std::shared_ptr<TXTLoader>;
}
#endif
