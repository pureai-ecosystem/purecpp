#pragma once

#include <vector>
#include <string>
#include <memory>
#include <optional>

#include "BaseLoader.h"

#include "lexbor/html/html.h"
#include "re2/re2.h"

namespace WebLoader
{
    const RE2 fontSizeRegex("font-size:\\s*([\\d\\.]+\\w+%?);?");
    
    class WebLoader : public DataLoader::BaseDataLoader
    {
    public:
        WebLoader() = delete;
        WebLoader(const std::vector<RAGLibrary::DataExtractRequestStruct>& urlsToScrap = {}, const int& numThreads = 0);
        ~WebLoader() = default;

        void InsertDataToExtract(const std::vector<RAGLibrary::DataExtractRequestStruct>& dataPaths) final;
    private:
        std::optional<std::string> ScrapURL(const std::string& url);
        void URLFontTextExtractor(const std::string& urlData, const std::string& urlPath);
        void ExtractPageTextElements(lxb_dom_node_t* node);
        void GetFontSize(lxb_dom_node_t* node, std::string& fontSize);

        mutable std::mutex m_mutex;
        std::vector<std::string> m_extractedText;
    };
    using WebLoaderPtr = std::shared_ptr<WebLoader>;
}