#include "WebLoader.h"

#include "beauty/beauty.hpp"
#include "RagException.h"

namespace WebLoader
{
    WebLoader::WebLoader(const std::vector<RAGLibrary::DataExtractRequestStruct>& urlsToScrap, const int& numThreads) :
        DataLoader::BaseDataLoader(numThreads)
    {
        if(!urlsToScrap.empty())
        {
            InsertDataToExtract(urlsToScrap);
        }

        AddThreadsCallback([this](RAGLibrary::DataExtractRequestStruct url){
            if (auto pageData = ScrapURL(url.targetIdentifier))
            {
                ExtractTextFromHTML(url.targetIdentifier, *pageData);
            }
        });
    }

    void WebLoader::InsertDataToExtract(const std::vector<RAGLibrary::DataExtractRequestStruct>& dataPaths)
    {
        InsertWorkIntoThreads(dataPaths);
    }

    std::optional<std::string> WebLoader::ScrapURL(const std::string& url)
    {
        beauty::client client;
        beauty::request req;
        req.set("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/130.0.0.0 Safari/537.36");
        req.method(beast::http::verb::get);

        auto [ec, response] = client.send_request(std::move(req), std::chrono::milliseconds(0), std::string(url));
        if (!ec)
        {
            if (response.status() == boost::beast::http::status::ok)
            {
                std::cout << std::format("Scrapped {}", url) << std::endl;
                return response.body();
            }
            else
            {
                std::cerr << std::format("Non OK status {} from response: {}({})", url, std::string(response.reason()), static_cast<int>(response.status())) << std::endl;
            }
        }
        else
        {
            std::cerr << std::format("Error when sending request to: {} error: {}", url, ec.message()) << std::endl;
        }

        return std::nullopt;
    }

    void WebLoader::ExtractTextFromHTML(const std::string& urlPath, const std::string& htmlData)
    {
        lxb_html_document_t* document = lxb_html_document_create();
        if (!document)
        {
            throw RAGLibrary::RagException("Failed to create HTML document");
        }

        auto status = lxb_html_document_parse(document, reinterpret_cast<unsigned char*>(const_cast<char*>(htmlData.c_str())), htmlData.size());
        if (status != LXB_STATUS_OK)
        {
            lxb_html_document_destroy(document);
            throw RAGLibrary::RagException(std::format("Failed to parse HTML document with status: {}", status));
        }

        RAGLibrary::Metadata metadata = { {"fileIdentifier", urlPath} };
        RAGLibrary::LoaderDataStruct dataStruct(metadata, {});

        if (auto* body = lxb_html_document_body_element(document); body != nullptr)
        {
            ExtractBodyText(lxb_dom_interface_node(body), dataStruct.textContent);
        }
        
        lxb_html_document_destroy(document);
        
        {
            std::lock_guard lock(m_mutex);
            m_dataVector.push_back(dataStruct);
        }
    }

    void WebLoader::ExtractBodyText(lxb_dom_node_t* node, std::vector<std::string>& textContent)
    {
        if (node->type == LXB_DOM_NODE_TYPE_TEXT)
        {
            auto* text = lxb_dom_node_text_content(node, nullptr);
            if (text && *text != '\0')
            {
                std::string textStr(reinterpret_cast<char*>(text));
                if (!textStr.empty() && !std::all_of(textStr.begin(), textStr.end(), isspace))
                {
                    textContent.push_back(textStr);
                }
            }
        }
        for (auto* child = lxb_dom_node_first_child(node); child; child = lxb_dom_node_next(child))
        {
            ExtractBodyText(child, textContent);
        }
    }
}