#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include "CommonStructs.h"

namespace MetadataExtractor
{
    struct Document
    {
        Document(const std::vector<std::string>& _pageContent, const std::vector<std::pair<std::string, std::string>>& _metadata = {}) :
            pageContent(_pageContent),
            metadata(_metadata)
            {}

        std::string StringRepr() 
        {
            std::string retString;
            for(auto index = metadata.begin(); index != metadata.end(); ++index)
            {
                retString += "Metadata: " + index->second + " To Chunk: " + index->first + "\n"; 
            }
            return retString;
        }

        std::vector<std::string> pageContent;
        std::vector<std::pair<std::string, std::string>> metadata;
    };
    using ThreadSafeQueueDocument = RAGLibrary::ThreadSafeQueue<Document>;
}