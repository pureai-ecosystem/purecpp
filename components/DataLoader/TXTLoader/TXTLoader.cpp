#include "TXTLoader.h"

#include <fstream>
#include <syncstream>
#include <filesystem>

#include <boost/algorithm/string.hpp>

#include "FileUtils.h"

namespace TXTLoader
{
    TXTLoader::TXTLoader(const std::vector<RAGLibrary::DataExtractRequestStruct>& filePaths, const unsigned int& numThreads) :
        DataLoader::BaseDataLoader(numThreads)
    {
        if(!filePaths.empty())
        {
            InsertDataToExtract(filePaths);
        }

        AddThreadsCallback([this](RAGLibrary::DataExtractRequestStruct filePath){
            ExtractTextFromTXT(filePath);
        });
    }

    void TXTLoader::InsertDataToExtract(const std::vector<RAGLibrary::DataExtractRequestStruct>& dataPaths)
    {
        LocalFileReader(dataPaths, ".txt");
    }

    void TXTLoader::ExtractTextFromTXT(const RAGLibrary::DataExtractRequestStruct& path)
    {
        auto verifyWhiteSpace = [](std::string str)
        {
            return std::all_of(str.begin(), str.end(), 
                [](auto ch) { 
                    return std::isspace(ch); 
                });
        };

        auto txtContent = RAGLibrary::FileReader(path.targetIdentifier);
        std::vector<std::string> result, cleanResult;

        boost::split(result, txtContent, boost::is_any_of("\n"));
        for(auto elem : result)
        {
    	    if(!verifyWhiteSpace(elem))
            {
                cleanResult.push_back(elem);
            }
        }

        if(!cleanResult.empty())
        {
            std::lock_guard lock(m_mutex);
            std::filesystem::path filePath(path.targetIdentifier);
            RAGLibrary::Metadata metadata = {{"fileIdentifier", filePath.filename().replace_extension("").c_str()}};
            m_dataVector.emplace_back(metadata, cleanResult);
        }
    }

}