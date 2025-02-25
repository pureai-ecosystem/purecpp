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
            if(auto pageData = ScrapURL(url.targetIdentifier))
            {
                URLFontTextExtractor(*pageData, url.targetIdentifier);
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
        if(!ec)
        {
            if(response.status() == boost::beast::http::status::ok)
            {
                std::cout << std::format("Scrapped {}", url) << std::endl;
                return response.body();
            }
            else
            {
                std::cerr << std::format("Non OK status {} from response: {}({})", url,std::string(response.reason()), static_cast<int>(response.status())) << std::endl;
            }
        }
        else
        {
            std::cerr << std::format("Erro when sending request to: {} error: {}", url, ec.message()) << std::endl;
        }

        return std::nullopt;
    }

    void WebLoader::URLFontTextExtractor(const std::string& urlData, const std::string& urlPath)
    {
        lxb_html_document_t *document = nullptr;
        try
        {
            document = lxb_html_document_create();
            if(document == nullptr)
            {
                throw(RAGLibrary::RagException("Failed to create HTML document"));
            }

            auto status = lxb_html_document_parse(document, reinterpret_cast<unsigned char*>(const_cast<char *>(urlData.c_str())), urlData.size());
            if(status != LXB_STATUS_OK)
            {
                throw(RAGLibrary::RagException(std::format("Failed to parse HTML document with status: {}", status)));
            }
            
            if(auto* body = lxb_html_document_body_element(document); body != nullptr)
            {
                ExtractPageTextElements(lxb_dom_interface_node(body));
                RAGLibrary::Metadata metadata = {{"fileIdentifier", urlPath}};
                m_dataVector.emplace_back(metadata, m_extractedText);
                m_extractedText.clear();
            }
            
            lxb_html_document_destroy(document);
        }
        catch(const RAGLibrary::RagException& e)
        {
            if(document != nullptr) lxb_html_document_destroy(document);
            std::cerr << e.what() << std::endl;
        }
    }

    void WebLoader::ExtractPageTextElements(lxb_dom_node_t* node)
    {
        std::string fontSize = "default";
        auto verifyWhiteSpace = [](std::string str)
        {
            return std::all_of(str.begin(), str.end(), 
                [](auto ch) { 
                    return std::isspace(ch); 
                });
        };

        if(node->type == LXB_DOM_NODE_TYPE_TEXT)
        {
            auto* t = lxb_dom_node_text_content(node, nullptr);
            if(t != nullptr)
            {
                std::string tStr = reinterpret_cast<char *>(const_cast<lxb_char_t *>(t));

                if(tStr.size() != 0 && !verifyWhiteSpace(tStr))
                {
                    GetFontSize(node->parent, fontSize);
                    {
                        std::lock_guard lock(m_mutex);
                        m_extractedText.push_back(std::format("font-size: {}, text: {}",fontSize, tStr));
                    }
                }
            }
        }
        auto* child = lxb_dom_node_first_child(node);
        while(child != nullptr)
        {
            ExtractPageTextElements(child);
            child = lxb_dom_node_next(child);
        }
    }

    void WebLoader::GetFontSize(lxb_dom_node_t* node, std::string& fontSize)
    {
        while(node)
        {
            if(node->type == LXB_DOM_NODE_TYPE_ELEMENT)
            {
                auto* elem = lxb_dom_interface_element(node);
                auto* attr = lxb_dom_element_get_attribute(elem, reinterpret_cast<lxb_char_t *>(const_cast<char *>("style")), 5, nullptr);
                if(attr)
                {
                    std::string style(reinterpret_cast<char *>(const_cast<lxb_char_t *>(attr)));

                    if(RE2::PartialMatch(style, fontSizeRegex, &fontSize))
                    {
                        return;
                    }
                }
            }
            node = lxb_dom_node_parent(node);
        }
    }



}