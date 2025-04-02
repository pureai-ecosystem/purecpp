#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>

#include "CommonStructs.h"

namespace DataLoader
{
    class IBaseDataLoader
    {
    public:
        virtual ~IBaseDataLoader() = default;

        virtual std::optional<RAGLibrary::LoaderDataStruct> GetTextContent(const std::string &fileIdentifier) = 0;
        virtual std::vector<RAGLibrary::DataStruct> Load() = 0;
        virtual bool KeywordExists(const std::string &fileName, const std::string &keyword) = 0;
        virtual RAGLibrary::UpperKeywordData GetKeywordOccurences(const std::string &keyword) = 0;
    };
    using IBaseDataLoaderPtr = std::shared_ptr<IBaseDataLoader>;

}