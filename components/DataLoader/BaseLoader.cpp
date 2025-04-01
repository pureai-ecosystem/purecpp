#include "BaseLoader.h"

#include <chrono>
#include <algorithm>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace DataLoader
{
    BaseDataLoader::BaseDataLoader(unsigned int threadsNum)
    {
        if (threadsNum)
        {
            m_isThreaded = true;
            m_threadQueues.resize(threadsNum);
            for (auto index = 0; index < threadsNum; ++index)
            {
                m_threadQueues[index].threadRunner = nullptr;
                m_threadQueues[index].threadRemainingWork = 0;
            }
        }
    }

    BaseDataLoader::~BaseDataLoader()
    {
        for (auto &elem : m_threadQueues)
        {
            while (elem.threadRemainingWork != 0)
                ;
        }
        m_killThread = true;
        m_threadYield.release();

        for (auto &elem : m_threadQueues)
        {
            if (elem.threadRunner != nullptr && elem.threadRunner->valid())
            {
                elem.threadRunner->wait();
            }
        }
    }

    void BaseDataLoader::AddThreadsCallback(std::function<void(RAGLibrary::DataExtractRequestStruct)> callback, std::function<void()> prefix, std::function<void()> suffix)
    {
        m_instanceCallback = callback;
        m_prefixCallback = prefix;
        m_suffixCallback = suffix;
        for (auto &elem : m_threadQueues)
        {
            elem.threadRunner = std::make_shared<std::future<void>>(std::async(std::launch::async, [this, callback, prefix, suffix, &elem]()
                                                                               {
                prefix();
                while(!m_killThread)
                {
                    if(auto value = elem.threadQueue.pop())
                    {
                        callback(*value);
                        elem.threadRemainingWork--;
                    }
                    m_threadYield.try_acquire_for(std::chrono::milliseconds(10));
                }
                suffix(); }));
        }
    }

    void BaseDataLoader::InsertWorkIntoThreads(const std::vector<RAGLibrary::DataExtractRequestStruct> &workload)
    {
        if (m_isThreaded)
        {
            WaitThreadsStartup();
            auto threadPoolSize = m_threadQueues.size();
            auto workloadSize = workload.size();

            for (auto index = 0; index < workloadSize; ++index)
            {
                auto newIndex = index % threadPoolSize;
                m_threadQueues[newIndex].threadQueue.push(workload[index]);
                m_threadQueues[newIndex].threadRemainingWork++;
            }
        }
        else
        {
            RAGLibrary::ThreadSafeQueueDataRequest queue(workload);
            m_prefixCallback();
            while (queue.size())
            {
                if (auto value = queue.pop())
                {
                    m_instanceCallback(*value);
                }
            }
            m_suffixCallback();
        }
    }

    std::optional<RAGLibrary::LoaderDataStruct> BaseDataLoader::Load()
    {
        std::cout << "Load" << std::endl;
        WaitFinishWorkload();
        for (auto &elem : m_dataVector)
        {
            std::cout << "Load: " << std::any_cast<std::string>(elem.metadata["fileIdentifier"]) << std::endl;
            return elem;
        }
        return std::nullopt;
    }

    std::optional<RAGLibrary::LoaderDataStruct> BaseDataLoader::GetTextContent(const std::string &fileName)
    {
        WaitFinishWorkload();
        for (auto &elem : m_dataVector)
        {
            if (std::any_cast<std::string>(elem.metadata["fileIdentifier"]) == fileName)
            {
                return elem;
            }
        }
        return std::nullopt;
    }

    bool BaseDataLoader::KeywordExists(const std::string &fileName, const std::string &keyword)
    {
        WaitFinishWorkload();

        for (const auto &elem : m_dataVector)
        {
            auto it = elem.metadata.find("fileIdentifier");
            if (it == elem.metadata.end() || std::any_cast<std::string>(it->second) != fileName)
                continue;

            for (const auto &page : elem.textContent)
                if (page.find(keyword) != std::string::npos)
                    return true;
        }

        return false;
    }

    RAGLibrary::UpperKeywordData BaseDataLoader::GetKeywordOccurences(const std::string &keyword)
    {
        WaitFinishWorkload();

        RAGLibrary::UpperKeywordData upperKeywordData;
        std::for_each(m_dataVector.begin(), m_dataVector.end(), [&upperKeywordData, keyword](auto &elem)
                      {
            for(auto line = 0; line < elem.textContent.size(); ++line)
            {
                auto pos = elem.textContent[line].find(keyword,0);
                std::string fileIdentifier = std::any_cast<std::string>(elem.metadata["fileIdentifier"]);
                while(pos != std::string::npos)
                {
                    upperKeywordData.totalOccurences++;

                    upperKeywordData.keywordDataPerFile[fileIdentifier].occurrences++;
                    upperKeywordData.keywordDataPerFile[fileIdentifier].position.emplace_back(line, static_cast<int>(pos));
                    pos = elem.textContent[line].find(keyword,pos + 1);
                } 
            } });
        return upperKeywordData;
    }

    void BaseDataLoader::WaitFinishWorkload()
    {
        for (auto &elem : m_threadQueues)
        {
            while (elem.threadRemainingWork > 0)
                ;
        }
    }

    void BaseDataLoader::LocalFileReader(const std::string &filePath, const std::string &extension)
    {
        std::vector<RAGLibrary::DataExtractRequestStruct> workQueue;
        auto regularFileProcessor = [this, &workQueue, extension](const fs::path &file)
        {
            if (fs::is_regular_file(file) && file.extension() == extension)
            {
                workQueue.emplace_back(file.string(), 0);
            }
        };
        {
            auto path = fs::path(filePath);
            if (fs::is_directory(path))
            {
                for (auto file : fs::recursive_directory_iterator(path))
                {
                    regularFileProcessor(file.path());
                }
            }
            else if (fs::is_regular_file(path))
            {
                regularFileProcessor(path);
            }
            InsertWorkIntoThreads(workQueue);
        }
    }

    void BaseDataLoader::WaitThreadsStartup()
    {
        for (auto index = 0; index < m_threadQueues.size(); ++index)
        {
            while (m_threadQueues[index].threadRunner == nullptr)
                ;
        }
    }
}