#pragma once

#include <vector>
#include <string>
#include <memory>
#include <optional>
#include <mutex>

#include "BaseLoader.h"
#include "lexbor/html/html.h"
#include "beauty/beauty.hpp"

namespace WebLoader
{
    class WebLoader : public DataLoader::BaseDataLoader
    {
    public:
        WebLoader() = delete;
        WebLoader(const std::vector<RAGLibrary::DataExtractRequestStruct> &urlsToScrap = {}, const int &numThreads = 0);
        ~WebLoader() = default;

        void InsertDataToExtract(const std::vector<RAGLibrary::DataExtractRequestStruct> &dataPaths) final;

    private:
        std::optional<std::string> ScrapURL(const std::string &url);
        void ExtractTextFromHTML(const std::string &urlPath, const std::string &htmlData);
        void ExtractBodyText(lxb_dom_node_t *node);

        mutable std::mutex m_mutex;
        std::vector<std::string> m_extractedText;
    };
    using WebLoaderPtr = std::shared_ptr<WebLoader>;
}
