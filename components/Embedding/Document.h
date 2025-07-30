#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <sstream>

#include "../MetadataExtractor/Document.h"

namespace Embedding
{
    struct Document
    {
        Document(const std::vector<std::string>& _pageContent, const std::vector<std::pair<std::string, std::string>>& _metadata = {}, const std::vector<float> _embeddings = {}) :
            pageContent(_pageContent),
            metadata(_metadata),
            embeddings(_embeddings)
            {}

        Document(const ::MetadataExtractor::Document& document)
        {
            this->pageContent = document.pageContent;
            this->metadata = document.metadata;
        }

        Document& operator=(const ::MetadataExtractor::Document& document)
        {
            this->pageContent = document.pageContent;
            this->metadata = document.metadata;
            return *this;
        }


        std::string StringRepr() 
        {
            std::stringstream retString;
            if(!metadata.empty())
            {
                for(auto index = 0; index < metadata.size(); ++index)
                {
                    retString << "Metadata: " << metadata[index].second << " To Chunk: " << metadata[index].first << std::endl; 
                }
            }
            if(!embeddings.empty())
            {
                retString << "Embeddings: \n" <<  "[ ";
                for(auto index = 0; index < embeddings.size(); ++index)
                {
                    retString << embeddings[index] << ", ";
                }
                retString.seekp(retString.str().size() - 2);
                retString << "]";

            }
            return retString.str();
        }

        std::vector<std::string> pageContent;
        std::vector<std::pair<std::string, std::string>> metadata;
        std::vector<float> embeddings;
    };
    using ThreadSafeQueueDocument = RAGLibrary::ThreadSafeQueue<Document>;
}
