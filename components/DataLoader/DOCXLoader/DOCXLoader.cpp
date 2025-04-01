#include "DOCXLoader.h"

#include "miniz.h"
#include "rapidxml.hpp"
#include "RagException.h"
#include "format"

namespace fs = std::filesystem;
namespace DOCXLoader
{
    DOCXLoader::DOCXLoader(const std::string filePath, const unsigned int &numThreads) : DataLoader::BaseDataLoader(numThreads)
    {
        AddThreadsCallback([this](RAGLibrary::DataExtractRequestStruct value)
                           {
            if (auto documentPath = ExtractZIPFile(value))
                ExtractTextFromXML(value.targetIdentifier, *documentPath); });

        if (!filePath.empty())
            LocalFileReader(filePath, ".docx");
    }

    std::optional<std::pair<std::string, int>> DOCXLoader::ExtractZIPFile(const RAGLibrary::DataExtractRequestStruct &path)
    {
        std::cout << "Extracting ZIP(mehtod) file: " << path.targetIdentifier << std::endl;
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
            std::cout << "Error(ZIP): " << e.what() << std::endl;
            std::cerr << e.what() << std::endl;
            return std::nullopt;
        }
    }

    void DOCXLoader::ExtractTextFromXML(fs::path filePath, const std::pair<std::string, int> &data)
    {
        std::cout << "Extracting text from XML" << std::endl;
        try
        {
            RAGLibrary::Metadata metadata = {{"source", filePath.string()}};
            RAGLibrary::DataStruct dataStruct(metadata, "");

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

            std::string fullText;
            for (rapidxml::xml_node<> *pNode = bodyNode->first_node("w:p"); pNode; pNode = pNode->next_sibling("w:p"))
            {
                std::string paragraph;
                for (rapidxml::xml_node<> *textNode = pNode->first_node("w:r"); textNode; textNode = textNode->next_sibling("w:r"))
                {
                    rapidxml::xml_node<> *tNode = textNode->first_node("w:t");
                    while (tNode)
                    {
                        paragraph += tNode->value();
                        tNode = tNode->next_sibling("w:t");
                    }
                }
                if (!paragraph.empty())
                {
                    fullText += paragraph + "\n";
                }
            }

            dataStruct.textContent = fullText;
        
            {
                std::lock_guard lock(m_mutex);
                m_dataVector.push_back(dataStruct);
            }
        }
        catch (const RAGLibrary::RagException &e)
        {
            std::cout << "Error(XML): " << e.what() << std::endl;
            std::cerr << e.what() << std::endl;
            throw;
        }
    }
}