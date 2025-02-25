#include "PDFLoader.h"

#include <filesystem>
#include <algorithm>
#include <iostream>
#include <format>
#include <iostream>

#include "fpdf_text.h"
#include "unicode/ustream.h"

#include "RagException.h"

namespace PDFLoader
{
    PDFLoader::PDFLoader(const std::vector<RAGLibrary::DataExtractRequestStruct>& filePaths, const unsigned int& numThreads) :
        DataLoader::BaseDataLoader(numThreads)
    {
        if(!filePaths.empty())
        {
            InsertDataToExtract(filePaths);
        }

        AddThreadsCallback([this](RAGLibrary::DataExtractRequestStruct filePath){
            ExtractPDFData(filePath);
        }, 
        [this](){ 
            FPDF_InitLibrary();
        }, 
        [this](){
            FPDF_DestroyLibrary(); 
        });
    }

    void PDFLoader::InsertDataToExtract(const std::vector<RAGLibrary::DataExtractRequestStruct>& dataPaths)
    {
        LocalFileReader(dataPaths, ".pdf");
    }

    void PDFLoader::ExtractPDFData(const RAGLibrary::DataExtractRequestStruct& path)
    {
        std::vector<std::string> extractedText;
        try
        {
            FPDF_DOCUMENT document;
            unsigned int pageLimit;
            {
                std::lock_guard lock(m_mutex);
                document = FPDF_LoadDocument(path.targetIdentifier.c_str(), nullptr);
                if(!document)
                {
                    throw RAGLibrary::RagException("Failed to open PDF file");
                }
            
                auto pageCount = FPDF_GetPageCount(document);
                pageLimit = path.extractContentLimit;
                if(pageLimit > pageCount)
                {
                    throw RAGLibrary::RagException("End page limit is bigger than total page size");
                }
                else if(pageLimit == 0)
                {
                    pageLimit = pageCount;
                }
                std::cout << std::format("Number of pages: {}", pageCount) << std::endl;
            }
            int numChars;
            FPDF_PAGE page;
            FPDF_TEXTPAGE textPage;
            for(auto pageIndex = 0; pageIndex < pageLimit; ++pageIndex)
            {
                std::scoped_lock lock(m_mutex);
                page = FPDF_LoadPage(document, pageIndex);
                if(!page)
                {
                    throw RAGLibrary::RagException("Failed to load page");
                }

                textPage = FPDFText_LoadPage(page);
                if(!textPage)
                {
                    FPDF_ClosePage(page);
                    throw RAGLibrary::RagException("Failed to load text page");
                }

                numChars = FPDFText_CountChars(textPage);

                std::string tmpPage;
                icu::UnicodeString unicodeStr;
                unsigned int unicodeChar;
                for(auto charIndex = 0; charIndex < numChars; ++charIndex)
                {
                    unicodeChar = FPDFText_GetUnicode(textPage, charIndex);
                    unicodeStr += static_cast<UChar32>(unicodeChar);
                }

                unicodeStr.toUTF8String(tmpPage);
                extractedText.push_back(tmpPage);

                FPDFText_ClosePage(textPage);
                FPDF_ClosePage(page);
            }

            {
                std::scoped_lock lock(m_mutex);
                FPDF_CloseDocument(document);
                std::filesystem::path file(path.targetIdentifier);
                RAGLibrary::Metadata metadata = {{"fileIdentifier", file.filename().replace_extension("").c_str()}};
                m_dataVector.emplace_back(metadata, extractedText);
            }
        }
        catch(const RAGLibrary::RagException& e)
        {
            std::cerr << e.what() << std::endl;
            throw;
        }
    }
}