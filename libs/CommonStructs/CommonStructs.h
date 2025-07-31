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
        DataExtractRequestStruct(const std::string &_targetIdentifier, const unsigned int &_extractContentLimit = 0)
            : targetIdentifier(_targetIdentifier), extractContentLimit(_extractContentLimit) {}

        std::string targetIdentifier;
        unsigned int extractContentLimit;
    };

    struct LoaderDataStruct
    {
        LoaderDataStruct(const Metadata &_metadata, const std::vector<std::string> &_textContent)
            : metadata(_metadata), textContent(_textContent) {}

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
        DataStruct(const Metadata &_metadata, const std::string &_textContent)
            : metadata(_metadata), textContent(_textContent) {}

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
            os << data.to_json().dump();
            return os;
        }
    };

    struct ThreadStruct
    {
        ThreadStruct() = default;
        ThreadStruct(std::shared_ptr<std::future<void>> _threadRunner, ThreadSafeQueueDataRequest _threadQueue, unsigned int _threadRemainingWork)
            : threadRunner(std::move(_threadRunner)), threadQueue(std::move(_threadQueue)), threadRemainingWork(_threadRemainingWork) {}

        ~ThreadStruct()
        {
            if (threadRunner && threadRunner->valid())
            {
                threadRunner->wait();
            }
        }

        ThreadStruct(const ThreadStruct &other) = default;
        ThreadStruct &operator=(const ThreadStruct &other) = default;

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
        KeywordData() : occurrences(0) {}

        int occurrences;
        std::vector<std::pair<int, int>> position;
    };

    struct UpperKeywordData
    {
        UpperKeywordData() : totalOccurences(0) {}

        int totalOccurences;
        std::map<std::string, KeywordData> keywordDataPerFile;

        friend std::ostream &operator<<(std::ostream &o, const UpperKeywordData &data)
        {
            o << "Total occurrences: " << data.totalOccurences << std::endl;
            for (const auto &elem : data.keywordDataPerFile)
            {
                o << "In File: " << elem.first << std::endl;
                o << "  Occurrences: " << elem.second.occurrences << std::endl;
                o << "  Positions: " << std::endl;
                for (const auto &[line, offset] : elem.second.position)
                {
                    o << "      [line: " << line << " offset: " << offset << "]" << std::endl;
                }
            }
            return o;
        }
    };

    static std::string meta2str(const Metadata &meta)
    {
        std::stringstream ss;
        bool first = true;
        ss << "{";
        for (const auto &pair : meta)
        {
            if (!first)
                ss << ",";
            else
                first = false;

            ss << "\"" << pair.first << "\":\"" << StringUtils::any2str(pair.second) << "\"";
        }
        ss << "}";
        return ss.str();
    }

    struct Document
    {
        Document() = default;
        Document(Metadata pmetadata, const std::string &ppage_content) : metadata(std::move(pmetadata)), page_content(ppage_content) {}
        Document(Metadata pmetadata, std::string ppage_content, std::vector<float> pembedding)
            : metadata(std::move(pmetadata)), page_content(std::move(ppage_content)), embedding(std::move(pembedding)) {}

        std::string StringRepr() const
        {
            std::stringstream ss;
            ss << *this;
            return ss.str();
        }

        Metadata metadata;
        std::string page_content;
        std::optional<std::vector<float>> embedding;

        std::size_t dim() const noexcept
        {
            return embedding ? embedding->size() : 0;
        }

        static std::string escape_with_json(const std::string &input)
        {
            std::string dumped = json(input).dump();
            return dumped.substr(1, dumped.size() - 2);
        }

        [[nodiscard]] std::string to_json() const;
        static Document from_json(std::string_view json);

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
