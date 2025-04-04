#include "BaseEmbedding.h"

#include <algorithm>

namespace Embedding
{
    Document BaseEmbedding::ProcessDocument(RAGLibrary::Document &document)
    {
        document.embeddings = GenerateEmbeddings({document});
        return document;
    }

    std::vector<Document> BaseEmbedding::ProcessDocuments(std::vector<RAGLibrary::Document> &documents, const int &maxWorkers)
    {
        std::vector<std::pair<std::shared_ptr<std::future<void>>, ThreadSafeQueueDocument>> threadPool(maxWorkers);
        std::atomic_bool killThread{false};
        std::binary_semaphore threadYield{0};
        std::mutex mutex;
        std::vector<Document> returnDocuments;

        for (auto &thread : threadPool)
        {
            thread.first = std::make_shared<std::future<void>>(std::async(std::launch::async, [this, &thread, &killThread, &threadYield, &returnDocuments, &mutex]()
                                                                          {
                while(!killThread)
                {
                    if(auto value = thread.second.pop())
                    {
                        std::scoped_lock lock(mutex);
                        returnDocuments.push_back(ProcessDocument(*value));
                    }
                    threadYield.try_acquire_for(std::chrono::milliseconds(10));
                } }));
        }

        for (auto index = 0; index < documents.size(); ++index)
        {
            threadPool[index % threadPool.size()].second.push(documents[index]);
        }

        std::for_each(threadPool.begin(), threadPool.end(), [this](auto &thread)
                      {
            while(thread.second.size() != 0); });
        killThread = true;
        threadYield.release();

        return returnDocuments;
    }
}