#include "DOCXLoader.h"
#include <format>

#include "miniz.h"
#include "rapidxml.hpp"
#include "RagException.h"

namespace DOCXLoader
{
    DOCXLoader::DOCXLoader(const std::vector<RAGLibrary::DataExtractRequestStruct> &filePaths, const unsigned int &numThreads) : DataLoader::BaseDataLoader(numThreads)
    {
        if (!filePaths.empty())
        {
            InsertDataToExtract(filePaths);
        }

        AddThreadsCallback([this](RAGLibrary::DataExtractRequestStruct value)
                           {
            if(auto documentPath = ExtractZIPFile(value))
            {
                ExtractTextFromXML(value.targetIdentifier, *documentPath);
            } });
    }

    void DOCXLoader::InsertDataToExtract(const std::vector<RAGLibrary::DataExtractRequestStruct>& dataPaths)
    {
        std::vector<RAGLibrary::DataExtractRequestStruct> workQueue; 
        auto regularFileProcessor = [this, &workQueue](const std::filesystem::path& dir, const unsigned int& docxPageLimit){
            if(std::filesystem::is_regular_file(dir) && dir.extension().string() == ".docx")
            {
                std::cout << std::format("IsRegularFile: {}", dir.string()) << std::endl;

                workQueue.emplace_back(std::string(dir.string()), docxPageLimit);
            }
        };
        std::for_each(dataPaths.begin(), dataPaths.end(), [this, regularFileProcessor](auto& str_path){
            auto path = std::filesystem::path(str_path.targetIdentifier);
            if(std::filesystem::is_directory(path))
            {
                for(auto dir : std::filesystem::recursive_directory_iterator{path})
                {
                    regularFileProcessor(dir.path(), str_path.extractContentLimit);
                }
            }
            else if(std::filesystem::is_regular_file(path))
            {
                regularFileProcessor(path, str_path.extractContentLimit);
            }
        });
        InsertWorkIntoThreads(workQueue);
    }

    std::optional<std::pair<std::string, int>> DOCXLoader::ExtractZIPFile(const RAGLibrary::DataExtractRequestStruct &path)
    {
        try
        {
            mz_zip_archive zipArchive = {};
            if (!mz_zip_reader_init_file(&zipArchive, path.targetIdentifier.c_str(), 0))
            {
                throw RAGLibrary::RagException("Failed to open ZIP archive");
            }

            auto fileIndex = mz_zip_reader_locate_file(&zipArchive, std::string(xmlDefaultPath).c_str(), nullptr, 0);
            if (fileIndex == -1)
            {
                mz_zip_reader_end(&zipArchive);
                throw RAGLibrary::RagException("File not found in ZIP archive");
            }

            size_t uncompressezSize = 0;
            auto fileContent = mz_zip_reader_extract_file_to_heap(&zipArchive, std::string(xmlDefaultPath).c_str(), &uncompressezSize, 0);
            if (!fileContent)
            {
                mz_zip_reader_end(&zipArchive);
                throw RAGLibrary::RagException("Failed to extract file from ZIP arichive: " + std::string(xmlDefaultPath));
            }

            std::string content(static_cast<char *>(fileContent), uncompressezSize);
            mz_free(fileContent);
            mz_zip_reader_end(&zipArchive);

            return std::pair(content, path.extractContentLimit);
        }
        catch (const RAGLibrary::RagException &e)
        {
            std::cerr << e.what() << std::endl;
            return std::nullopt;
        }
    }

    void DOCXLoader::ExtractTextFromXML(std::filesystem::path filePath, const std::pair<std::string, int> &data)
    {
        try
        {
            RAGLibrary::Metadata metadata = {{"fileIdentifier", filePath.filename().replace_extension("").string()}};
            RAGLibrary::LoaderDataStruct dataStruct(metadata, {});

            rapidxml::xml_document<> document;
            std::vector<char> xmlCopy(data.first.begin(), data.first.end());
            xmlCopy.push_back('\0');

            document.parse<0>(xmlCopy.data());

            rapidxml::xml_node<> *root = document.first_node("w:document");
            if (!root)
            {
                throw RAGLibrary::RagException("Invalid XML: Missing 'w:document' node");
            }

            rapidxml::xml_node<> *bodyNode = root->first_node("w:body");
            if (!bodyNode)
            {
                throw RAGLibrary::RagException("Invalid XML: Missing 'w:body' node");
            }

            std::string page("");
            for (rapidxml::xml_node<> *pNode = bodyNode->first_node("w:p"); pNode; pNode = pNode->next_sibling("w:p"))
            {
                for (rapidxml::xml_node<> *textNode = pNode->first_node("w:r"); textNode; textNode = textNode->next_sibling("w:r"))
                {
                    rapidxml::xml_node<> *tNode = textNode->first_node("w:t");

                    std::string line("");
                    while (tNode)
                    {
                        line += tNode->value() + std::string("\n");
                        tNode = tNode->next_sibling("w:t");
                    }
                    if (!line.empty())
                    {
                        page += line;
                    }
                }
                if (!page.empty())
                {
                    dataStruct.textContent.push_back(page);
                    page = "";
                }
            }
            {
                std::lock_guard lock(m_mutex);
                m_dataVector.push_back(dataStruct);
            }
        }
        catch (const RAGLibrary::RagException &e)
        {
            std::cerr << e.what() << std::endl;
            throw;
        }
    }

}