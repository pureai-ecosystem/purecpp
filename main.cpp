#include <iostream>
#include <syncstream>
/*
#include "ChunkCount/ChunkCount.h"
#include "ChunkDefault/ChunkDefault.h"
#include "ContentCleaner/ContentCleaner.h"
#include "DOCXLoader/DOCXLoader.h"
#include "MetadataRegexExtractor/MetadataRegexExtractor.h"
#include "MetadataHFExtractor/MetadataHFExtractor.h"
#include "EmbeddingOpenAI/EmbeddingOpenAI.h"
#include "PDFLoader/PDFLoader.h"
#include "StringUtils.h"
#include "TXTLoader/TXTLoader.h"
#include "WebLoader/WebLoader.h"

#include "beauty/beauty.hpp"

int main()
{
    DataLoader::IBaseDataLoaderPtr pdfLoader = std::make_shared<PDFLoader::PDFLoader>(std::vector<RAGLibrary::DataExtractRequestStruct>{}, 12);
    DataLoader::IBaseDataLoaderPtr docxLoader = std::make_shared<DOCXLoader::DOCXLoader>(std::vector<RAGLibrary::DataExtractRequestStruct>{}, 12);
    DataLoader::IBaseDataLoaderPtr webLoader = std::make_shared<WebLoader::WebLoader>(std::vector<RAGLibrary::DataExtractRequestStruct>{}, 12);
    DataLoader::IBaseDataLoaderPtr txtLoader = std::make_shared<TXTLoader::TXTLoader>(std::vector<RAGLibrary::DataExtractRequestStruct>{}, 12);
    MetadataRegexExtractor::IMetadataRegexExtractorPtr metadataExtractor = std::make_shared<MetadataRegexExtractor::MetadataRegexExtractor>();
    MetadataHFExtractor::IMetadataHFExtractorPtr metadataHFExtractor = std::make_shared<MetadataHFExtractor::MetadataHFExtractor>();
    EmbeddingOpenAI::IEmbeddingOpenAIPtr embeddingOpenAI = std::make_shared<EmbeddingOpenAI::EmbeddingOpenAI>();

    metadataHFExtractor->InitializeNERModel();
    embeddingOpenAI->SetAPIKey(EmbeddingOpenAI::APIKeyOpenAI);

    MetadataExtractor::Document document({"testing@email.com", "(75)4423-5675", "01/12/2020", "font-size: 80%"});
    MetadataExtractor::Document document2({"Mahatma Gandhi", "sdfsfsf",  "53", "https://www.google.com", "font-size: 43px"});
    MetadataExtractor::Document document3({"2453", "Elvis Presley"});
    MetadataExtractor::Document document4({"My name is Wolfgang and I live in Berlin"});


    auto returnedDocs = metadataHFExtractor->ProcessDocuments({document, document2, document3, document4}, 4);
    auto embeddedDocs = embeddingOpenAI->ProcessDocuments(std::vector<Embedding::Document>(returnedDocs.begin(), returnedDocs.end()), 4);

    for(auto doc : embeddedDocs)
    {
        std::cout << "Embedding response: \n" << doc.StringRepr() << std::endl;
    }

    metadataExtractor->AddPattern("FontSize", "font-size:\\s*([\\d\\.]+\\w+%?);?");

    pdfLoader->InsertDataToExtract({RAGLibrary::DataExtractRequestStruct("pdfs")});

    docxLoader->InsertDataToExtract({RAGLibrary::DataExtractRequestStruct("docxs")});

    webLoader->InsertDataToExtract({RAGLibrary::DataExtractRequestStruct("https://g1.globo.com/ciencia/noticia/2024/12/02/por-que-missao-da-india-para-exploracao-do-sol-e-crucial-para-o-mundo.ghtml"), RAGLibrary::DataExtractRequestStruct("https://pt.wikipedia.org/wiki/%C3%8Dndia")});

    txtLoader->InsertDataToExtract({RAGLibrary::DataExtractRequestStruct("txts")});

    std::osyncstream(std::cout) << docxLoader->GetKeywordOccurences("video") << std::endl;
    std::osyncstream(std::cout) << pdfLoader->GetKeywordOccurences("Rutherford") << std::endl;
    std::osyncstream(std::cout) << webLoader->GetKeywordOccurences("Índia") << std::endl;
    if(auto value = webLoader->GetTextContent("https://pt.wikipedia.org/wiki/%C3%8Dndia"))
    {
        for(auto str : value->textContent)
        {
            std::osyncstream(std::cout) <<  str << std::endl;
        }
    }

    if(auto value = txtLoader->GetTextContent("conanman"))
    {
        for(auto str : value->textContent)
        {
            std::osyncstream(std::cout) << str << std::endl;
        }
    }

    document = metadataExtractor->ProcessDocument(document);
    std::cout << document.StringRepr() << std::endl;

    auto docs = metadataExtractor->ProcessDocuments({document, document2, document3}, 6);
    for(auto elem : docs)
    {
        std::cout << elem.StringRepr() << std::endl;
    }

    std::vector<std::string> lines;
    std::string fileIdentifier;
    if(auto value = txtLoader->GetTextContent("sample24"))
    {
        lines = value->textContent;
        fileIdentifier = value->fileIdentifier;
    }

    RAGLibrary::LoaderDataStruct input(fileIdentifier, lines);

    auto chunkCount = Chunk::ChunkCount("coordinated", 600, 2);
    auto documents = chunkCount.ProcessSingleDocument(input);
    std::osyncstream(std::cout) <<  "ChunkCount('coordinated', 600, 2) from '"<< fileIdentifier << "': documents.size=" << documents.size() << std::endl;

    auto chunkDefault = Chunk::ChunkDefault(100, 20);
    documents = chunkDefault.ProcessSingleDocument(input);
    std::osyncstream(std::cout) <<  "ChunkDefault(100, 20) from '"<< fileIdentifier << "': documents.size=" << documents.size() << std::endl;

    std::string text;
    StringUtils::joinStr("\n", lines, text);

    RAGLibrary::Metadata metadata = {{"fileIdentifier", fileIdentifier}};
    RAGLibrary::Document doc4(metadata, text);

    std::osyncstream(std::cout) <<  "doc.page_content.size=" << doc4.page_content.size() << std::endl;

    CleanData::ContentCleaner cleaner;
    auto otherDocument = cleaner.ProcessDocument(doc4, {"Click"});

    std::osyncstream(std::cout) <<  "ContentCleaner from '"<< fileIdentifier << "': document.page_content.size=" << otherDocument.page_content.size() << std::endl;

    beauty::signal({SIGINT, SIGTERM, SIGHUP}, [](int s){
        std::cout << std::format("Gracefully stopping with signal {}", s) << std::endl;
        beauty::stop();
    });
    beauty::wait();

    return 0;
}*/

#include <iostream>
#include "PDFLoader/PDFLoader.h"
#include "DOCXLoader/DOCXLoader.h"
#include "CommonStructs.h"
#include "ChunkQuery/ChunkQuery.h"
#include "ChunkCommons/ChunkCommons.h"

int main()
{
    Chunk::ChunkQuery chunkQuery = Chunk::ChunkQuery(100, 20, Chunk::EmbeddingModel::HuggingFace, "");
    RAGLibrary::Metadata metadata;
    metadata["author"] = std::string("John Doe");
    metadata["version"] = 1;

    // Criando um vetor de texto válido
    std::vector<std::string> textContent = {"Texto de exemplo 1", "Texto de exemplo 2"};

    // Inicializando LoaderDataStruct corretamente
    metadata
    RAGLibrary::Document item(metadata, textContent);
    RAGLibrary::
    std::vector<float> query_embedding;
    float similarity_threshold = 0.8f;

    // std::vector<RAGLibrary::Document> result1 = chunkQuery.ProcessSingleDocument(item, query_embedding, similarity_threshold);
    chunkQuery.ProcessDocuments({item}, "example query", similarity_threshold, 4);

    std::vector<RAGLibrary::LoaderDataStruct> items;
    std::string query = "example query";
    int max_workers = 4;

    // std::vector<RAGLibrary::Document> result2 = chunkQuery.ProcessDocuments(items, query, similarity_threshold, max_workers);
}