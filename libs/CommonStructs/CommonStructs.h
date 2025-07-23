#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <future>
#include <sstream>
#include <map>
#include <any>
#include <nlohmann/json.hpp>

#include <semaphore>
#include <format>
#include <fstream>
#include "ThreadSafeQueue.h"
#include "StringUtils.h"
#include "MUtils.h"

using json = nlohmann::json;
namespace RAGLibrary
{
    struct DataExtractRequestStruct;
    struct LoaderDataStruct;
    struct ThreadStruct;
    struct KeywordData;
    struct UpperKeywordData;
    struct Document;

    using ThreadSafeQueueDataRequest = ThreadSafeQueue<DataExtractRequestStruct>;
    using Metadata = std::map<std::string, std::string>;

    struct DataExtractRequestStruct
    {
        DataExtractRequestStruct() = default;
        DataExtractRequestStruct(const std::string &_targetIdentifier, const unsigned int &_extractContentLimit = 0) : targetIdentifier(_targetIdentifier),
                                                                                                                       extractContentLimit(_extractContentLimit)
        {}

        std::string targetIdentifier;
        unsigned int extractContentLimit;
    };

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

    struct DataStruct
    {
        DataStruct(const Metadata &_metadata, const std::string &_textContent) : metadata(_metadata),
                                                                                 textContent(_textContent)
        {
        }

        DataStruct(const DataStruct &other) = default;
        DataStruct &operator=(const DataStruct &other) = default;

        Metadata metadata;
        std::string textContent;

        json to_json() const
        {
            return json{{"metadata", metadata}, {"textContent", textContent}};
        }

        friend std::ostream &operator<<(std::ostream &os, const DataStruct &data)
        {
            os << data.to_json().dump(); // Retorna JSON como string
            return os;
        }

        // friend std::ostream &operator<<(std::ostream &os, const DataStruct &data)
        // {
        //     os << "(metadata={";

        //     auto it = data.metadata.begin();
        //     while (it != data.metadata.end())
        //     {
        //         os << it->first << ": " << it->second;
        //         if (++it != data.metadata.end())
        //         {
        //             os << ", ";
        //         }
        //     }

        //     os << "}, textContent=\"" << data.textContent << "\")";
        //     return os;
        // }
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
        std::optional<std::vector<float>> embedding;

        static std::string escape_with_json(const std::string &input)
        {
            std::string dumped = nlohmann::json(input).dump();
            return dumped.substr(1, dumped.size() - 2); // remove aspas externas
        }

        friend std::ostream &operator<<(std::ostream &o, const Document &data)
        {
            o << "Document(metadata=" << meta2str(data.metadata)
              << ", page_content=\"" << StringUtils::ellipsis(escape_with_json(data.page_content)) << "\"";

            if (data.embedding)
            {
                o << ", embedding=[";
                const auto &emb = *data.embedding;
                for (size_t i = 0; i < emb.size(); ++i)
                {
                    if (i > 0)
                        o << ", ";
                    o << emb[i];
                    if (i >= 4)
                    {
                        o << ", ...";
                        break;
                    }
                }
                o << "]";
            }

            o << ")";
            return o;
        }
    };

}
