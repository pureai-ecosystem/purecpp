#ifndef WEB_LOADER_H
#define WEB_LOADER_H

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
        WebLoader(const std::string url, const int &numThreads = 1);
        ~WebLoader() = default;

    private:
        std::optional<std::string> ScrapURL(const std::string &url);
        void ExtractTextFromHTML(const std::string &urlPath, const std::string &htmlData);
        void ExtractBodyText(lxb_dom_node_t *node);

        mutable std::mutex m_mutex;
        std::string m_extractedText;
    };
    using WebLoaderPtr = std::shared_ptr<WebLoader>;
}
#endif
