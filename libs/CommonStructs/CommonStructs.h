#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <future>
#include <sstream>
#include <map>
#include <any>

#include "ThreadSafeQueue.h"
#include "StringUtils.h"

namespace RAGLibrary
{
    struct DataExtractRequestStruct;
    struct LoaderDataStruct;
    struct ThreadStruct;
    struct KeywordData;
    struct UpperKeywordData;
    struct Document;

    struct DataExtractRequestStruct
    {
        DataExtractRequestStruct() = default;
        DataExtractRequestStruct(const std::string &_targetIdentifier, const unsigned int &_extractContentLimit = 0) : targetIdentifier(_targetIdentifier),
                                                                                                                       extractContentLimit(_extractContentLimit)
        {
        }

        std::string targetIdentifier;
        unsigned int extractContentLimit;
    };

    using ThreadSafeQueueDataRequest = ThreadSafeQueue<DataExtractRequestStruct>;
    using Metadata = std::map<std::string, std::string>;

    struct LoaderDataStruct
    {
        LoaderDataStruct(const Metadata &_metadata, const std::vector<std::string> &_textContent) : metadata(_metadata),
                                                                                                    textContent(_textContent)
        {
        }

        LoaderDataStruct(const LoaderDataStruct &other) = default;
        LoaderDataStruct &operator=(const LoaderDataStruct &other) = default;

        friend std::ostream &operator<<(std::ostream &o, const LoaderDataStruct &data)
        {
            for (auto index = 0; index < data.textContent.size(); ++index)
            {
                o << "  SubGroup: " << index + 1 << std::endl;
                o << "  TextContent: " << std::endl
                  << data.textContent[index] << std::endl;
            }
            o << std::endl;
            return o;
        }

        Metadata metadata;
        std::vector<std::string> textContent;
    };

    struct ThreadStruct
    {
        ThreadStruct() = default;
        ThreadStruct(std::shared_ptr<std::future<void>> _threadRunner, ThreadSafeQueueDataRequest _threadQueue, unsigned int _threadRemainingWork) : threadRunner(std::move(_threadRunner)),
                                                                                                                                                     threadQueue(_threadQueue),
                                                                                                                                                     threadRemainingWork(_threadRemainingWork)
        {
        }
        ~ThreadStruct()
        {
            if (threadRunner != nullptr && threadRunner->valid())
            {
                threadRunner->wait();
            }
        }

        ThreadStruct(const ThreadStruct &other) = default;
        ThreadStruct &operator=(ThreadStruct &other) = default;

        bool operator<(const ThreadStruct &other) const
        {
            return this->threadRemainingWork < other.threadRemainingWork;
        }

        std::shared_ptr<std::future<void>> threadRunner;
        ThreadSafeQueueDataRequest threadQueue;
        unsigned int threadRemainingWork;
    };

    struct KeywordData
    {
        KeywordData() : occurrences(0)
        {
        }

        int occurrences;
        std::vector<std::pair<int, int>> position;
    };

    struct UpperKeywordData
    {
        UpperKeywordData() : totalOccurences(0)
        {
        }

        friend std::ostream &operator<<(std::ostream &o, const UpperKeywordData &data)
        {
            o << "Total occurrences: " << data.totalOccurences << std::endl;
            for (auto elem : data.keywordDataPerFile)
            {
                o << "In File: " << elem.first << std::endl;
                o << "  Occurrences: " << elem.second.occurrences << std::endl;
                o << "  Positions: " << std::endl;
                for (auto lower_index = 0; lower_index < elem.second.position.size(); ++lower_index)
                {
                    o << "      [line: " << elem.second.position[lower_index].first << " offset: "
                      << elem.second.position[lower_index].second << "]" << std::endl;
                }
            }
            return o;
        }
        int totalOccurences;
        std::map<std::string, KeywordData> keywordDataPerFile;
    };

    static std::string meta2str(const Metadata &meta)
    {
        std::stringstream ss;
        bool first = true;
        ss << "{";
        for (auto &pair : meta)
        {
            if (first)
                first = false;
            else
                ss << ",";

            auto key = pair.first;
            auto value = StringUtils::any2str(pair.second);

            ss << "\"" << key << "\":\"" << value << "\"";
        }
        ss << "}";
        return ss.str();
    }

    struct Document
    {

        Document() = default;
        Document(Metadata pmetadata, const std::string &ppage_content) : metadata(pmetadata), page_content(ppage_content)
        {
        }

        std::string StringRepr()
        {
            std::stringstream ss;
            operator<<(ss, *this);
            return ss.str();
        }

        Metadata metadata;
        std::string page_content;

        friend std::ostream &operator<<(std::ostream &o, const Document &data)
        {
            const auto &page_content = data.page_content;
            o << "Document(" << "metadata=" << meta2str(data.metadata) << ", page_content=\"" << StringUtils::ellipsis(page_content) << "\""
                                                                                                                                        ")";
            return o;
        }
    };

}