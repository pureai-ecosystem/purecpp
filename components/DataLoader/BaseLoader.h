#pragma once

#include "IBaseLoader.h"
#include <future>
#include <functional>
#include <atomic>
#include <utility>
#include <semaphore>

namespace DataLoader
{
    class BaseDataLoader : public IBaseDataLoader
    {
    protected:
        // BaseDataLoader() = delete;
        // BaseDataLoader(unsigned int threadsNum);
        void AddThreadsCallback(std::function<void(RAGLibrary::DataExtractRequestStruct)> callback, std::function<void()> prefix = []() {}, std::function<void()> suffix = []() {});
        void InsertWorkIntoThreads(const std::vector<RAGLibrary::DataExtractRequestStruct> &workload);
        void WaitFinishWorkload();
        void LocalFileReader(const std::string &dataPaths, const std::string &extension);
        std::vector<RAGLibrary::DataStruct> m_dataVector;

    public:
        BaseDataLoader() = delete;
        BaseDataLoader(unsigned int threadsNum);
        virtual ~BaseDataLoader();
        std::optional<RAGLibrary::LoaderDataStruct> GetTextContent(const std::string &pdfFileName) final;
        std::vector<RAGLibrary::DataStruct> Load() final;
        bool KeywordExists(const std::string &pdfFileName, const std::string &keyword) final;
        RAGLibrary::UpperKeywordData GetKeywordOccurences(const std::string &keyword) final;

    private:
        std::atomic_bool m_killThread{false};
        std::binary_semaphore m_threadYield{0};
        std::vector<RAGLibrary::ThreadStruct> m_threadQueues;
        std::function<void(RAGLibrary::DataExtractRequestStruct)> m_instanceCallback;
        std::function<void()> m_prefixCallback;
        std::function<void()> m_suffixCallback;
        bool m_isThreaded{false};

        void WaitThreadsStartup();
    };
    using BaseDataLoaderPtr = std::shared_ptr<BaseDataLoader>;
}