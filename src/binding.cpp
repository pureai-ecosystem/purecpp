
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <pybind11/complex.h>
#include <pybind11/chrono.h>
#include <optional>
#include <memory>
#include <future>
#include <string>
#include <sstream>
#include <utility>
#include <vector>
#include <algorithm>
#include <any>
#include <map>
#include <torch/torch.h>
#include <re2/re2.h>

#include "RagException.h"
#include "ThreadSafeQueue.h"
#include "StringUtils.h"
#include "FileUtilsLocal.h"
#include "CommonStructs.h"

#include "IBaseLoader.h"
#include "BaseLoader.h"
#include "PDFLoader/PDFLoader.h"
#include "DOCXLoader/DOCXLoader.h"
#include "TXTLoader/TXTLoader.h"
#include "WebLoader/WebLoader.h"

#include "ContentCleaner/ContentCleaner.h"

#include "ChunkDefault/ChunkDefault.h"
#include "ChunkCount/ChunkCount.h"
#include "ChunkSimilarity/ChunkSimilarity.h"
#include "ChunkCommons/ChunkCommons.h"
#include "ChunkQuery/ChunkQuery.h"

#include "../components/MetadataExtractor/Document.h"
#include "IMetadataExtractor.h"
#include "MetadataExtractor.h"
#include "MetadataRegexExtractor/IMetadataRegexExtractor.h"
#include "MetadataHFExtractor/IMetadataHFExtractor.h"
#include "MetadataRegexExtractor/MetadataRegexExtractor.h"
#include "MetadataHFExtractor/MetadataHFExtractor.h"

#include "../components/Embedding/Document.h"
#include "IBaseEmbedding.h"
#include "BaseEmbedding.h"
#include "EmbeddingOpenAI/IEmbeddingOpenAI.h"
#include "EmbeddingOpenAI/EmbeddingOpenAI.h"

namespace py = pybind11;
using namespace RAGLibrary;
using namespace DataLoader;

typedef std::vector<std::pair<std::string, std::string>> test_vec_pair;
// using test_vec_pair as std::vector<std::pair<std::string, std::string>>;

// Do not use using namespace PDFLoader; to avoid conflicts.
// The class will be referenced using full qualification.

//--------------------------------------------------------------------------
// Binding function for RagException
//--------------------------------------------------------------------------
void bind_RagException(py::module &m)
{
    py::register_exception<RagException>(m, "RagException");
}

void bind_FileUtilsLocal(py::module &m)
{
    m.def("FileReader", &RAGLibrary::FileReader,
          py::arg("filePath"),
          R"doc(
              Reads the content of a file specified by 'filePath'.
    Returns a std::string containing the entire file content.
    In case of an error, throws a RagException.
          )doc");
}

void bind_StringUtils(py::module &m)
{
    m.def("escapeRegex", &StringUtils::escapeRegex, py::arg("str"),
          "Escapes special characters in a string for use in regular expressions..");

    // For joinStr, we created a small lambda to return the resulting string
    // since the original function fills it by reference.
    m.def("joinStr", [](const std::string &str, const std::vector<std::string> &inputs)
          {
        std::string output;
        StringUtils::joinStr(str, inputs, output);
        return output; }, py::arg("str"), py::arg("inputs"), "Joins a vector of strings with a separator provided in 'str'.");

    m.def("ellipsis", &StringUtils::ellipsis, py::arg("text"), py::arg("maxLength") = 100,
          "Truncates the string and adds ellipsis if it exceeds 'maxLength'.'.");

    m.def("any2str", &StringUtils::any2str, py::arg("var"),
          "Converts a std::any into a representative string.");

    m.def("str_details", &StringUtils::str_details, py::arg("text"),
          "Returns details of the provided string.");
    m.def("removeAccents", &StringUtils::removeAccents, py::arg("input"),
          "Removes accents from the provided string.");
}
//--------------------------------------------------------------------------
// Template for ThreadSafeQueue.
//--------------------------------------------------------------------------
template <typename Type>
void bindThreadSafeQueue(py::module &m, const std::string &name)
{
    py::class_<ThreadSafeQueue<Type>>(m, name.c_str())
        .def(py::init<>())
        .def(py::init<const ThreadSafeQueue<Type> &>())
        .def(py::init<const std::vector<Type> &>(), py::arg("vect"))
        .def("push", &ThreadSafeQueue<Type>::push, py::arg("value"))
        .def("pop", &ThreadSafeQueue<Type>::pop)
        .def("size", &ThreadSafeQueue<Type>::size)
        .def("clear", &ThreadSafeQueue<Type>::clear);
}

void bind_CommonStructs(py::module &m)
{
    py::class_<RAGLibrary::DataExtractRequestStruct>(m, "DataExtractRequestStruct")
        .def(py::init<>())
        .def(py::init<const std::string &, unsigned int>(),
             py::arg("targetIdentifier"), py::arg("extractContentLimit") = 0)
        .def_readwrite("targetIdentifier", &RAGLibrary::DataExtractRequestStruct::targetIdentifier)
        .def_readwrite("extractContentLimit", &RAGLibrary::DataExtractRequestStruct::extractContentLimit);

    bindThreadSafeQueue<RAGLibrary::DataExtractRequestStruct>(m, "ThreadSafeQueueDataRequest");
    bindThreadSafeQueue<std::string>(m, "ThreadSafeQueueString");

    py::class_<RAGLibrary::LoaderDataStruct>(m, "LoaderDataStruct")
        .def(py::init<const RAGLibrary::Metadata &, const std::vector<std::string> &>(),
             py::arg("metadata"), py::arg("textContent"))
        .def_readwrite("metadata", &RAGLibrary::LoaderDataStruct::metadata)
        .def_readwrite("textContent", &RAGLibrary::LoaderDataStruct::textContent)
        .def("__str__", [](const RAGLibrary::LoaderDataStruct &data)
             {
            std::ostringstream o;
            o << data; // Uses the overloaded << operator
            return o.str(); });

    py::class_<RAGLibrary::DataStruct>(m, "DataStruct")
        .def(py::init<const RAGLibrary::Metadata &, const std::string &>(),
             py::arg("metadata"), py::arg("textContent"))
        .def_readwrite("metadata", &RAGLibrary::DataStruct::metadata)
        .def_readwrite("textContent", &RAGLibrary::DataStruct::textContent);

    py::class_<RAGLibrary::ThreadStruct>(m, "ThreadStruct")
        .def(py::init<>())
        .def(py::init<std::shared_ptr<std::future<void>>, RAGLibrary::ThreadSafeQueueDataRequest, unsigned int>(),
             py::arg("threadRunner"), py::arg("threadQueue"), py::arg("threadRemainingWork"))
        .def_readwrite("threadRunner", &RAGLibrary::ThreadStruct::threadRunner)
        .def_readwrite("threadQueue", &RAGLibrary::ThreadStruct::threadQueue)
        .def_readwrite("threadRemainingWork", &RAGLibrary::ThreadStruct::threadRemainingWork);

    py::class_<RAGLibrary::KeywordData>(m, "KeywordData")
        .def(py::init<>())
        .def_readwrite("occurrences", &RAGLibrary::KeywordData::occurrences)
        .def_readwrite("position", &RAGLibrary::KeywordData::position);

    py::class_<RAGLibrary::UpperKeywordData>(m, "UpperKeywordData")
        .def(py::init<>())
        .def_readwrite("totalOccurences", &RAGLibrary::UpperKeywordData::totalOccurences)
        .def_readwrite("keywordDataPerFile", &RAGLibrary::UpperKeywordData::keywordDataPerFile)
        .def("__str__", [](const RAGLibrary::UpperKeywordData &data)
             {
            std::ostringstream o;
            o << data; // // Uses the overloaded << operator
            return o.str(); });

    // Binding for RAGLibrary::Document
    // Here we assume that the Metadata type is already a std::map<std::string, std::any> supported by pybind11.
    // Otherwise, a custom converter for std::any may be required.
    py::class_<RAGLibrary::Document>(m, "RAGDocument")
        .def(py::init<>())
        .def(py::init<RAGLibrary::Metadata, const std::string &>(),
             py::arg("metadata"), py::arg("page_content"))
        .def_readwrite("metadata", &RAGLibrary::Document::metadata)
        .def_readwrite("page_content", &RAGLibrary::Document::page_content)
        .def("StringRepr", &RAGLibrary::Document::StringRepr)
        .def("__str__", [](const RAGLibrary::Document &doc)
             {
            std::ostringstream o;
            o << doc; 
            return o.str(); });
}

//--------------------------------------------------------------------------
// Trampoline class for IBaseDataLoader.
//--------------------------------------------------------------------------
class PyIBaseDataLoader : public IBaseDataLoader
{
public:
    std::optional<LoaderDataStruct> GetTextContent(const std::string &fileIdentifier) override
    {
        PYBIND11_OVERRIDE_PURE(
            std::optional<LoaderDataStruct>,
            IBaseDataLoader,
            GetTextContent,
            fileIdentifier);
    }

    std::optional<DataStruct> Load() override
    {
        PYBIND11_OVERRIDE_PURE(
            std::optional<DataStruct>,
            IBaseDataLoader,
            Load);
    }

    bool KeywordExists(const std::string &fileName, const std::string &keyword) override
    {
        PYBIND11_OVERRIDE_PURE(
            bool,
            IBaseDataLoader,
            KeywordExists,
            fileName, keyword);
    }

    UpperKeywordData GetKeywordOccurences(const std::string &keyword) override
    {
        PYBIND11_OVERRIDE_PURE(
            UpperKeywordData,
            IBaseDataLoader,
            GetKeywordOccurences,
            keyword);
    }
};

void bind_IBaseDataLoader(py::module &m)
{
    py::class_<IBaseDataLoader, PyIBaseDataLoader, IBaseDataLoaderPtr>(m, "IBaseDataLoader")
        .def(py::init<>())
        .def("GetTextContent", &IBaseDataLoader::GetTextContent, py::arg("fileIdentifier"))
        .def("Load", &IBaseDataLoader::Load)
        .def("KeywordExists", &IBaseDataLoader::KeywordExists, py::arg("fileName"), py::arg("keyword"))
        .def("GetKeywordOccurences", &IBaseDataLoader::GetKeywordOccurences, py::arg("keyword"));
}
// --------------------------------------------------------------------------
// Binding for FileUtilsLocal.
// --------------------------------------------------------------------------

//--------------------------------------------------------------------------
// Trampoline class for BaseDataLoader.
//--------------------------------------------------------------------------
class PyBaseDataLoader : public BaseDataLoader
{
public:
    PyBaseDataLoader(unsigned int threadsNum)
        : BaseDataLoader(threadsNum) {}
};

void bind_BaseDataLoader(py::module &m)
{
    py::class_<BaseDataLoader, PyBaseDataLoader, std::shared_ptr<BaseDataLoader>, IBaseDataLoader>(m, "BaseDataLoader")
        .def(py::init<unsigned int>(), py::arg("threadsNum"))
        .def("GetTextContent", &BaseDataLoader::GetTextContent, py::arg("pdfFileName"))
        .def("Load", &BaseDataLoader::Load)
        .def("KeywordExists", &BaseDataLoader::KeywordExists, py::arg("pdfFileName"), py::arg("keyword"))
        .def("GetKeywordOccurences", &BaseDataLoader::GetKeywordOccurences, py::arg("keyword"));
}

//--------------------------------------------------------------------------
// Binding for ContentCleaner.
//--------------------------------------------------------------------------
void bind_ContentCleaner(py::module &m)
{
    py::class_<CleanData::ContentCleaner>(m, "ContentCleaner")
        .def(py::init<const std::vector<std::string> &>(), py::arg("default_patterns") = std::vector<std::string>{})
        .def("ProcessDocument", &CleanData::ContentCleaner::ProcessDocument,
             py::arg("doc"), py::arg("custom_patterns") = std::vector<std::string>{})
        .def("ProcessDocuments", &CleanData::ContentCleaner::ProcessDocuments,
             py::arg("docs"), py::arg("custom_patterns") = std::vector<std::string>{}, py::arg("max_workers") = 4);
}
/*
void bind_EmbeddingModel(py::module &m) {
    py::enum_<Chunk::EmbeddingModel>(m, "EmbeddingModel", R"doc(
        Supported embedding models for similarity calculation:

        - HuggingFace
        - OpenAI
    )doc")
        .value("HuggingFace", Chunk::EmbeddingModel::HuggingFace)
        .value("OpenAI", Chunk::EmbeddingModel::OpenAI)
        .export_values();
}
*/

//--------------------------------------------------------------------------
// Binding function for ChunkCommon.
//--------------------------------------------------------------------------
void bind_ChunkCommons(py::module &m)
{
    //--------------------------------------------------------------------------
    // Binding for the EmbeddingModel enum.
    //--------------------------------------------------------------------------

    py::enum_<Chunk::EmbeddingModel>(m, "EmbeddingModel", R"doc(
        Enumeration of supported embedding models:

        HuggingFace: Uses models from HuggingFace.
        OpenAI: Uses models from OpenAI.
    )doc")
        .value("HuggingFace", Chunk::EmbeddingModel::HuggingFace)
        .value("OpenAI", Chunk::EmbeddingModel::OpenAI)
        .export_values();

    //--------------------------------------------------------------------------
    // Binding for the MeanPooling function.
    //--------------------------------------------------------------------------
    m.def("MeanPooling", &Chunk::MeanPooling,
          py::arg("token_embeddings"), py::arg("attention_mask"), py::arg("embedding_size"),
          R"doc(
              Computes the average of embeddings based on the attention mask.

                Parameters:

                token_embeddings (list[float]): List of token embeddings.
                attention_mask (list[int]): List of attention masks (1 or 0).
                embedding_size (int): Size of the embeddings.
                Returns:

                list[float]: List of averaged embeddings.
          )doc");

    //--------------------------------------------------------------------------
    // Binding for the NormalizeEmbeddings function
    //--------------------------------------------------------------------------
    m.def("NormalizeEmbeddings", &Chunk::NormalizeEmbeddings,
          py::arg("embeddings"),
          R"doc(
              Normalizes the embeddings so that they have a norm of 1.

                Parameters:

                embeddings (list[float]): List of embeddings to be normalized.
          )doc");

    //--------------------------------------------------------------------------
    // Binding for the EmbeddingModelBatch function.
    //--------------------------------------------------------------------------
    m.def("EmbeddingModelBatch", &Chunk::EmbeddingModelBatch,
          py::arg("chunks"), py::arg("model"), py::arg("batch_size") = 32,
          R"doc(
              Generates embeddings for a batch of chunks using a specified model.

                Parameters:

                chunks (list[str]): List of strings to be embedded.
                model (str): Name of the model to be used.
                batch_size (int, optional): Batch size (default=32).
                Returns:

                list[list[float]]: List of lists of embeddings.
          )doc");

    //--------------------------------------------------------------------------
    // Binding for the EmbeddingHuggingFaceTransformers function.
    //--------------------------------------------------------------------------
    m.def("EmbeddingHuggingFaceTransformers", &Chunk::EmbeddingHuggingFaceTransformers,
          py::arg("chunks"),
          R"doc(
              Generates embeddings using the HuggingFace model 'sentence-transformers/all-MiniLM-L6-v2'.

                Parameters:

                chunks (list[str]): List of strings to be embedded.
                Returns:

                list[list[float]]: List of lists of embeddings.
          )doc");

    //--------------------------------------------------------------------------
    // Binding for the EmbeddingOpenAI function.
    //--------------------------------------------------------------------------
    m.def("EmbeddingOpeanAI", &Chunk::EmbeddingOpeanAI,
          py::arg("chunks"), py::arg("openai_api_key"),
          R"doc(
              Generates embeddings using the OpenAI API.

                Parameters:

                chunks (list[str]): List of strings to be embedded.
                openai_api_key (str): OpenAI API key.
                Returns:

                list[list[float]]: List of lists of embeddings.
          )doc");

    //--------------------------------------------------------------------------
    // Binding for the toTensor function.
    //--------------------------------------------------------------------------
    m.def("toTensor", &Chunk::toTensor,
          py::arg("vect"),
          R"doc(
              Converts a list of lists of floats into a PyTorch tensor.

                Parameters:

                vect (list[list[float]]): List of lists of floats.
                Returns:

                torch.Tensor: PyTorch tensor.
          )doc");

    //--------------------------------------------------------------------------
    // Binding for the SplitText function.
    //--------------------------------------------------------------------------
    m.def("SplitText", &Chunk::SplitText,
          py::arg("inputs"), py::arg("overlap"), py::arg("chunk_size"),
          R"doc(
              Splits the text into overlapping chunks.

                Parameters:

                inputs (list[str]): List of input strings.
                overlap (int): Number of characters for overlap.
                chunk_size (int): Size of each chunk.
                Returns:

                list[str]: List of resulting chunks.
          )doc");

    //--------------------------------------------------------------------------
    // Binding for the SplitTextByCount function.
    //--------------------------------------------------------------------------
    m.def("SplitTextByCount", &Chunk::SplitTextByCount,
          py::arg("inputs"), py::arg("overlap"), py::arg("count_threshold"), py::arg("regex"),
          R"doc(
              Splits the text into chunks based on count and regex.

                Parameters:

                inputs (list[str]): List of input strings.
                overlap (int): Number of characters for overlap.
                count_threshold (int): Count threshold for splitting.
                regex (re2.RE2): Regular expression to identify split points.
                Returns:

                list[str]: List of resulting chunks.
          )doc");
}

//--------------------------------------------------------------------------
// Binding for ChunkDefault.
//--------------------------------------------------------------------------
void bind_ChunkDefault(py::module &m)
{
    py::class_<Chunk::ChunkDefault>(m, "ChunkDefault")
        .def(py::init<const int, const int>(), py::arg("chunk_size") = 100, py::arg("overlap") = 20)
        .def("ProcessSingleDocument", &Chunk::ChunkDefault::ProcessSingleDocument, py::arg("item"))
        .def("ProcessDocuments", &Chunk::ChunkDefault::ProcessDocuments,
             py::arg("items"), py::arg("max_workers") = 4);
}

//--------------------------------------------------------------------------
// Binding for ChunkCount.
//--------------------------------------------------------------------------
void bind_ChunkCount(py::module &m)
{
    py::class_<Chunk::ChunkCount>(m, "ChunkCount")
        .def(py::init<const std::string &, const int, const int>(),
             py::arg("count_unit"), py::arg("overlap") = 600, py::arg("count_threshold") = 1)
        .def(py::init<>())
        .def("ProcessSingleDocument", &Chunk::ChunkCount::ProcessSingleDocument, py::arg("item"))
        .def("ProcessDocuments", &Chunk::ChunkCount::ProcessDocuments,
             py::arg("items"), py::arg("max_workers") = 4);
}
// --------------------------------------------------------------------------
// Binding for Chunk::ChunkSimilarity.
// --------------------------------------------------------------------------

/**
 * Creates the binding for the ChunkSimilarity class, responsible for splitting
 *text into chunks and generating embeddings for similarity analysis.
 */
void bind_ChunkSimilarity(py::module &m)
{
    // First, we create the binding for the EmbeddingModel enum (if it hasn't been created yet).
    // If you prefer, you can call bind_EmbeddingModel(m); inside the PYBIND11_MODULE,
    // but keeping it here may be more convenient.

    py::class_<Chunk::ChunkSimilarity>(m, "ChunkSimilarity", R"doc(
        Class for processing LoaderDataStruct and generating chunks and embeddings,
        allowing document similarity evaluation. It includes options to define chunk size,
        overlap, embedding model (HuggingFace or OpenAI), and an API key for OpenAI if needed.
    )doc")
        .def(
            py::init<int, int, Chunk::EmbeddingModel, const std::string &>(),
            py::arg("chunk_size") = 100,
            py::arg("overlap") = 20,
            py::arg("embedding_model") = Chunk::EmbeddingModel::HuggingFace,
            py::arg("openai_api_key") = "",
            R"doc(
                Constructor that initializes the ChunkSimilarity class.

                Parameters:

                chunk_size (int): Default size of each text chunk (default=100).
                overlap (int): Overlap between successive chunks (default=20).
                embedding_model (EmbeddingModel): Embedding model (HuggingFace or OpenAI).
                openai_api_key (str): OpenAI API key (only required if embedding_model=OpenAI).
            )doc")
        .def(
            "ProcessSingleDocument",
            &Chunk::ChunkSimilarity::ProcessSingleDocument,
            py::arg("item"),
            R"doc(
                Given a single LoaderDataStruct, splits its content into chunks
                and generates embeddings according to the chosen model, returning
                a vector of RAGLibrary::Document.

                Parameters:

                item (RAGLibrary.LoaderDataStruct): Structure containing
                the identifier and textual content.
                Returns:

                list[RAGDocument]: Vector of resulting documents,
                each with chunked content and possibly embeddings
                associated in its metadata.
            )doc")
        .def(
            "ProcessDocuments",
            &Chunk::ChunkSimilarity::ProcessDocuments,
            py::arg("items"),
            py::arg("max_workers") = 4,
            R"doc(
                Given a single LoaderDataStruct, splits its content into chunks
                and generates embeddings according to the chosen model, returning
                a vector of RAGLibrary::Document.

                Parameters:

                item (RAGLibrary.LoaderDataStruct): Structure containing
                the identifier and textual content.
                Returns:

                list[RAGDocument]: Vector of resulting documents,
                each with chunked content and possibly embeddings
                associated in its metadata.
            )doc");
}
// --------------------------------------------------------------------------
// Binding for PDFLoader
// --------------------------------------------------------------------------
// Note: We are using full qualification for the PDFLoader class.
void bind_PDFLoader(py::module &m)
{
    py::class_<::PDFLoader::PDFLoader, std::shared_ptr<::PDFLoader::PDFLoader>, DataLoader::BaseDataLoader>(m, "PDFLoader")
        .def(py::init<const std::string, const unsigned int &>(),
             py::arg("filePath"),
             py::arg("numThreads") = 1,
             "Creates a PDFLoader with a file path and an optional number of threads.");
}
// The function for DOCXLoader
void bind_DOCXLoader(py::module &m)
{
    py::class_<::DOCXLoader::DOCXLoader, std::shared_ptr<::DOCXLoader::DOCXLoader>, DataLoader::BaseDataLoader>(m, "DOCXLoader")
        .def(py::init<const std::string, const unsigned int &>(),
             py::arg("filePath"),
             py::arg("numThreads") = 1,
             "Creates a DOCXLoader with a file path and an optional number of threads.");
}

// Binding function for TXTLoader
void bind_TXTLoader(py::module &m)
{
    py::class_<::TXTLoader::TXTLoader, std::shared_ptr<::TXTLoader::TXTLoader>, DataLoader::BaseDataLoader>(m, "TXTLoader")
        .def(py::init<const std::string, const unsigned int &>(),
             py::arg("filePath"),
             py::arg("numThreads") = 1,
             "Creates a TXTLoader, optionally with initial paths and a defined number of threads.");
}

// Binding function for WebLoader
void bind_WebLoader(py::module &m)
{
    py::class_<::WebLoader::WebLoader, std::shared_ptr<::WebLoader::WebLoader>, DataLoader::BaseDataLoader>(m, "WebLoader")
        .def(py::init<const std::string, const unsigned int &>(),
             py::arg("url"),
             py::arg("numThreads") = 1,
             "Creates a WebLoader with optional URLs and a defined number of threads.");
}
// --------------------------------------------------------------------------
// Binding for MetadataExtractor::Document
// --------------------------------------------------------------------------
// 1) Remove map<int, string> from the binding and use vector<pair<string, string>> instead.
void bind_Document(py::module &m)
{
    py::class_<::MetadataExtractor::Document>(m, "Document")
        .def(
            py::init<
                const std::vector<std::string> &,
                const std::vector<std::pair<std::string, std::string>> &>(),
            py::arg("pageContent"),
            py::arg("metadata") = std::vector<std::pair<std::string, std::string>>{},
            R"doc(
                Constructor of Document.

                Parameters:

                pageContent (list[str]): List of strings representing the document content.
                metadata (list[tuple[str, str]]): List of key-value pairs for metadata.
            )doc")
        .def_readwrite("pageContent", &::MetadataExtractor::Document::pageContent, R"doc(
                Field that stores the document content as a vector of strings.
            )doc")
        .def_readwrite("metadata", &::MetadataExtractor::Document::metadata, R"doc(
                Document metadata, stored as key-value pairs.
            )doc")
        .def("StringRepr", &::MetadataExtractor::Document::StringRepr,
             R"doc(
                Returns a string representation listing each metadata entry in the document.
            )doc");

    bindThreadSafeQueue<::MetadataExtractor::Document>(m, "ThreadSafeQueueDocument");
}

// Trampoline class for IMetadataExtractor
class PyIMetadataExtractor : public MetadataExtractor::IMetadataExtractor
{
public:
    ::MetadataExtractor::Document ProcessDocument(::MetadataExtractor::Document doc) override
    {
        PYBIND11_OVERRIDE_PURE(
            ::MetadataExtractor::Document,
            MetadataExtractor::IMetadataExtractor,
            ProcessDocument,
            doc);
    }

    std::vector<::MetadataExtractor::Document> ProcessDocuments(std::vector<::MetadataExtractor::Document> docs, const int &maxWorkers) override
    {
        PYBIND11_OVERRIDE_PURE(
            std::vector<::MetadataExtractor::Document>,
            MetadataExtractor::IMetadataExtractor,
            ProcessDocuments,
            docs, maxWorkers);
    }
};

void bind_IMetadataExtractor(py::module &m)
{
    py::class_<MetadataExtractor::IMetadataExtractor, PyIMetadataExtractor, MetadataExtractor::IMetadataExtractorPtr>(m, "IMetadataExtractor")
        .def(py::init<>())
        .def("ProcessDocument", &MetadataExtractor::IMetadataExtractor::ProcessDocument, py::arg("doc"))
        .def("ProcessDocuments", &MetadataExtractor::IMetadataExtractor::ProcessDocuments, py::arg("docs"), py::arg("maxWorkers"));
}
// --------------------------------------------------------------------------
// Binding for MetadataExtractor::MetadataExtractor
// --------------------------------------------------------------------------
// Trampoline class to allow method overrides in Python
class PyMetadataExtractor : public MetadataExtractor::MetadataExtractor
{
public:
    using MetadataExtractor::MetadataExtractor::MetadataExtractor;

    //  Python implementation for a pure virtual method
    ::MetadataExtractor::Document ProcessDocument(::MetadataExtractor::Document doc) override
    {
        PYBIND11_OVERRIDE_PURE(
            ::MetadataExtractor::Document,          // Return type
            ::MetadataExtractor::MetadataExtractor, // Parent class
            ProcessDocument,                        // Name of function
            doc                                     // Parameters
        );
    }

    // Python implementation for a virtual method.
    std::vector<::MetadataExtractor::Document> ProcessDocuments(std::vector<::MetadataExtractor::Document> docs, const int &maxWorkers) override
    {
        PYBIND11_OVERRIDE(
            std::vector<::MetadataExtractor::Document>, // Return type
            ::MetadataExtractor::MetadataExtractor,     // Parent class
            ProcessDocuments,                           // Name of Function
            docs,                                       // Parameter 1
            maxWorkers                                  // Parameter 2
        );
    }
};

void bind_MetadataExtractor(py::module &m)
{
    py::class_<MetadataExtractor::MetadataExtractor, PyMetadataExtractor, std::shared_ptr<MetadataExtractor::MetadataExtractor>, MetadataExtractor::IMetadataExtractor>(
        m,
        "MetadataExtractor",
        R"doc(
            Base class for metadata extraction, inherited from IMetadataExtractor.
            It includes a pure virtual method to process a single document and
            a method to process multiple documents in parallel.
        )doc")
        .def(
            py::init<>(),
            R"doc(
                Default constructor for the MetadataExtractor class.
        )doc")
        .def(
            "ProcessDocument",
            &MetadataExtractor::MetadataExtractor::ProcessDocument,
            py::arg("doc"),
            R"doc(
            Processes metadata for a single document.
            This method is pure virtual and must be overridden
            in concrete derived classes
        )doc")
        .def(
            "ProcessDocuments",
            &MetadataExtractor::MetadataExtractor::ProcessDocuments,
            py::arg("docs"),
            py::arg("maxWorkers") = 4,
            R"doc(
            Processes metadata for multiple documents with parallelism support.
            By default, it uses up to 4 threads (maxWorkers=4).
        )doc");
}

// Trampoline class for IMetadataRegexExtractor
class PyIMetadataRegexExtractor : public MetadataRegexExtractor::IMetadataRegexExtractor
{
public:
    // Pure virtual methods of IMetadataRegexExtractor
    void AddPattern(const std::string &name, const std::string &pattern) override
    {
        PYBIND11_OVERRIDE_PURE(
            void,
            MetadataRegexExtractor::IMetadataRegexExtractor,
            AddPattern,
            name, pattern);
    }

    // Methods inherited from IMetadataExtractor
    ::MetadataExtractor::Document ProcessDocument(::MetadataExtractor::Document doc) override
    {
        PYBIND11_OVERRIDE_PURE(
            ::MetadataExtractor::Document,
            MetadataRegexExtractor::IMetadataRegexExtractor,
            ProcessDocument,
            doc);
    }

    std::vector<::MetadataExtractor::Document> ProcessDocuments(std::vector<::MetadataExtractor::Document> docs, const int &maxWorkers) override
    {
        PYBIND11_OVERRIDE_PURE(
            std::vector<::MetadataExtractor::Document>,
            MetadataRegexExtractor::IMetadataRegexExtractor,
            ProcessDocuments,
            docs, maxWorkers);
    }
};

void bind_IMetadataRegexExtractor(py::module &m)
{
    py::class_<MetadataRegexExtractor::IMetadataRegexExtractor, PyIMetadataRegexExtractor, MetadataRegexExtractor::IMetadataRegexExtractorPtr, MetadataExtractor::IMetadataExtractor>(m, "IMetadataRegexExtractor")
        .def(py::init<>())
        .def("AddPattern", &MetadataRegexExtractor::IMetadataRegexExtractor::AddPattern, py::arg("name"), py::arg("pattern"))
        .def("ProcessDocument", &MetadataRegexExtractor::IMetadataRegexExtractor::ProcessDocument, py::arg("doc"))
        .def("ProcessDocuments", &MetadataRegexExtractor::IMetadataRegexExtractor::ProcessDocuments, py::arg("docs"), py::arg("maxWorkers"));
}

// --------------------------------------------------------------------------
// Binding for `MetadataHFExtractor::IMetadataHFExtractor`.
// --------------------------------------------------------------------------

class PyIMetadataHFExtractor : public MetadataHFExtractor::IMetadataHFExtractor
{
public:
    using MetadataHFExtractor::IMetadataHFExtractor::IMetadataHFExtractor;

    void InitializeNERModel() override
    {
        PYBIND11_OVERRIDE_PURE(
            void,
            MetadataHFExtractor::IMetadataHFExtractor,
            InitializeNERModel);
    }

    test_vec_pair ExtractMetadata(const std::vector<std::string> &text) override
    {
        PYBIND11_OVERRIDE_PURE(
            test_vec_pair,
            MetadataHFExtractor::IMetadataHFExtractor,
            ExtractMetadata,
            text);
    }

    ::MetadataExtractor::Document ProcessDocument(::MetadataExtractor::Document doc) override
    {
        PYBIND11_OVERRIDE_PURE(
            ::MetadataExtractor::Document,
            MetadataHFExtractor::IMetadataHFExtractor,
            ProcessDocument,
            doc);
    }
};

/**
 * Creates the binding for the `IMetadataHFExtractor` interface. Since it is a
 * purely virtual class, we need the `PyIMetadataHFExtractor` trampoline
 * to allow methods to be overridden in Python.
 */

void bind_IMetadataHFExtractor(py::module &m)
{
    py::class_<MetadataHFExtractor::IMetadataHFExtractor,
               PyIMetadataHFExtractor,
               std::shared_ptr<MetadataHFExtractor::IMetadataHFExtractor>,
               ::MetadataExtractor::MetadataExtractor>(
        m,
        "IMetadataHFExtractor",
        R"doc(
            Interface that inherits from MetadataExtractor::MetadataExtractor and adds
            methods to initialize a NER model and extract metadata.
        )doc")
        .def(
            py::init<>(),
            R"doc(
            Default constructor for the IMetadataHFExtractor interface.
            Note that it cannot be instantiated without a concrete subclass.
        )doc")
        .def(
            "InitializeNERModel",
            &MetadataHFExtractor::IMetadataHFExtractor::InitializeNERModel,
            R"doc(
            Pure method that must be overridden to load and initialize
            the Named Entity Recognition (NER) model.
        )doc")
        .def(
            "ExtractMetadata",
            &MetadataHFExtractor::IMetadataHFExtractor::ExtractMetadata,
            py::arg("text"),
            R"doc(
            Pure method to extract metadata (named entities) from one or more texts.
            Returns a vector of pairs (token, entity).
        )doc")
        .def(
            "ProcessDocument",
            &MetadataHFExtractor::IMetadataHFExtractor::ProcessDocument,
            py::arg("doc"),
            R"doc(
            Pure method that processes metadata in a MetadataExtractor::Document.
            It may include NER logic.
        )doc");
}

void bind_MetadataRegexExtractor(py::module &m)
{
    py::class_<MetadataRegexExtractor::MetadataRegexExtractor, std::shared_ptr<MetadataRegexExtractor::MetadataRegexExtractor>, MetadataRegexExtractor::IMetadataRegexExtractor>(m, "MetadataRegexExtractor")
        .def(py::init<>())
        .def("AddPattern", &MetadataRegexExtractor::MetadataRegexExtractor::AddPattern, py::arg("name"), py::arg("pattern"))
        .def("ProcessDocument", &MetadataRegexExtractor::MetadataRegexExtractor::ProcessDocument, py::arg("doc"))
        .def("ProcessDocuments", &MetadataRegexExtractor::MetadataRegexExtractor::ProcessDocuments, py::arg("docs"), py::arg("maxWorkers"));
}

// --------------------------------------------------------------------------
// Binding for MetadataHFExtractor::MetadataHFExtractor.
// --------------------------------------------------------------------------

void bind_MetadataHFExtractor(py::module &m)
{
    py::class_<MetadataHFExtractor::MetadataHFExtractor,
               std::shared_ptr<MetadataHFExtractor::MetadataHFExtractor>,
               MetadataHFExtractor::IMetadataHFExtractor>(
        m,
        "MetadataHFExtractor",
        R"doc(
            Concrete class that implements metadata extraction using NER models via ONNXRuntime and tokenization libraries (HuggingFace tokenizers).
        )doc")
        .def(
            py::init<>(),
            R"doc(
            Default constructor that sets up the ONNXRuntime environment, sessions, and
            necessary tokenizers for metadata extraction.
        )doc")
        .def(
            "InitializeNERModel",
            &MetadataHFExtractor::MetadataHFExtractor::InitializeNERModel,
            R"doc(
            Initializes the NER model by loading the ONNX file and preparing
            the inference environment (ONNXRuntime session, CPU provider, etc.)
        )doc")
        .def(
            "ExtractMetadata",
            &MetadataHFExtractor::MetadataHFExtractor::ExtractMetadata,
            py::arg("text"),
            R"doc(
            Performs metadata extraction (named entities) from one or more
            provided texts. Returns a vector of pairs (entity, label).

            Parameters:

            text (list[str]): List of strings to be processed.
            Returns:

            list[tuple[str, str]]: Each element is a pair (token, entity).
        )doc")
        .def(
            "ProcessDocument",
            &MetadataHFExtractor::MetadataHFExtractor::ProcessDocument,
            py::arg("doc"),
            R"doc(
            Processes metadata in a MetadataExtractor.Document object, including
            named entity detection. Returns the same Document object, but
            with updated metadata.

            Parameters:

            doc (MetadataExtractor.Document): Document to be processed.
            Returns:

            MetadataExtractor.Document: Document with named entities
            added to its metadata set.
        )doc");
}
// --------------------------------------------------------------------------
// Binding para Embedding::Document
// --------------------------------------------------------------------------

/**
 **To avoid conflicts with the 'Document' class from other namespaces (RAGLibrary,
 **MetadataExtractor), we will expose this structure as 'EmbeddingDocument' in Python.
 */
void bind_EmbeddingDocument(py::module &m)
{
    // ----------------------------------------------------------------------
    // Class Embedding::Document --> Python: EmbeddingDocument
    // ----------------------------------------------------------------------
    py::class_<Embedding::Document>(
        m,
        "EmbeddingDocument",
        R"doc(
            Document structure for embeddings, containing:

           - pageContent: Vector of strings representing the content.
           - metadata: Vector of key-value pairs related to the content.
           - embeddings: Vector of floats representing the embedding vectors.
        )doc")
        .def(
            py::init<
                const std::vector<std::string> &,
                const std::vector<std::pair<std::string, std::string>> &,
                const std::vector<float> &>(),
            py::arg("pageContent"),
            py::arg("metadata") = std::vector<std::pair<std::string, std::string>>{},
            py::arg("embeddings") = std::vector<float>{},
            R"doc(
            Main constructor that takes:

            - pageContent (list[str]): List of content.
            - metadata (list[tuple[str, str]]): List of key-value pairs as metadata.
            - embeddings (list[float]): Embeddings (vector of floats).
        )doc")
        .def(
            py::init<const ::MetadataExtractor::Document &>(),
            py::arg("document"),
            R"doc(
            Constructor that converts a MetadataExtractor::Document
            into an EmbeddingDocument.
        )doc")
        .def(
            "StringRepr",
            &Embedding::Document::StringRepr,
            R"doc(
            Returns a textual representation of the document,
            including metadata and embeddings if they exist.
        )doc")
        .def_readwrite("pageContent", &Embedding::Document::pageContent)
        .def_readwrite("metadata", &Embedding::Document::metadata)
        .def_readwrite("embeddings", &Embedding::Document::embeddings);

    // ----------------------------------------------------------------------
    // Optional: Binding for ThreadSafeQueue<Embedding::Document>
    // ----------------------------------------------------------------------
    py::class_<Embedding::ThreadSafeQueueDocument>(
        m,
        "ThreadSafeQueueEmbeddingDocument",
        R"doc(
            Thread-safe queue of EmbeddingDocument, allowing concurrent access
            in parallelism scenarios.
        )doc")
        .def(py::init<>())
        .def("push", &Embedding::ThreadSafeQueueDocument::push, py::arg("value"))
        .def("pop", &Embedding::ThreadSafeQueueDocument::pop)
        .def("size", &Embedding::ThreadSafeQueueDocument::size);
}
// --------------------------------------------------------------------------
// Binding for Embedding::IBaseEmbedding
// --------------------------------------------------------------------------

/**
 * Trampoline class (Python wrapper) for IBaseEmbedding, allowing
 * the overriding of pure virtual methods in Python.
 */
class PyIBaseEmbedding : public Embedding::IBaseEmbedding
{
public:
    ~PyIBaseEmbedding() = default;

    std::vector<float> GenerateEmbeddings(const std::vector<std::string> &text) override
    {
        PYBIND11_OVERRIDE_PURE(
            std::vector<float>,        // Type class
            Embedding::IBaseEmbedding, // Base class
            GenerateEmbeddings,        // Name of method
            text                       // Parameters
        );
    }

    Embedding::Document ProcessDocument(Embedding::Document document) override
    {
        PYBIND11_OVERRIDE_PURE(
            Embedding::Document,       // Type class
            Embedding::IBaseEmbedding, // Base class
            ProcessDocument,           // Name of method
            document                   // Parameters
        );
    }

    std::vector<Embedding::Document> ProcessDocuments(std::vector<Embedding::Document> documents, const int &maxWorkers) override
    {
        PYBIND11_OVERRIDE_PURE(
            std::vector<Embedding::Document>, // Type class
            Embedding::IBaseEmbedding,        // Base class
            ProcessDocuments,                 // Nome do método
            documents,                        // Name of method
            maxWorkers                        // Parameters
        );
    }
};

/**
 * Creates the binding for the `IBaseEmbedding` interface.
 * Note: Since it is an interface with pure methods,
 * it cannot be instantiated directly without a concrete class,
 * but we can inherit in Python using the trampoline class.
 */

void bind_IBaseEmbedding(py::module &m)
{
    py::class_<Embedding::IBaseEmbedding, PyIBaseEmbedding, Embedding::IBaseEmbeddingPtr>(
        m,
        "IBaseEmbedding",
        R"doc(
            Base interface for generating embeddings in texts or documents.
            It includes methods for generating embeddings, processing a document,
            and processing multiple documents in parallel.
        )doc")
        .def(
            py::init<>(),
            R"doc(
            Default constructor for the IBaseEmbedding interface.
            It cannot be instantiated without overriding methods in Python.
        )doc")
        .def(
            "GenerateEmbeddings",
            &Embedding::IBaseEmbedding::GenerateEmbeddings,
            py::arg("text"),
            R"doc(
            Generates embeddings for a list of strings.

            Parameters:

            text (list[str]): List of texts to be converted into embeddings.
            Returns:

            list[float]: Vector of floats representing the concatenated embeddings.
        )doc")
        .def(
            "ProcessDocument",
            &Embedding::IBaseEmbedding::ProcessDocument,
            py::arg("document"),
            R"doc(
            Generates embeddings and inserts them into a specific `EmbeddingDocument` object.

            **Parameters:**  
            - `document` (`EmbeddingDocument`): The document to be processed.

            **Returns:**  
            - `EmbeddingDocument`: Document with the embeddings populated.
        )doc")
        .def(
            "ProcessDocuments",
            &Embedding::IBaseEmbedding::ProcessDocuments,
            py::arg("documents"),
            py::arg("maxWorkers") = 4,
            R"doc(
            Processes multiple documents in parallel, generating embeddings for each.

            Parameters:

            documents (list[EmbeddingDocument]): List of documents to process.
            maxWorkers (int): Maximum number of threads used in processing (default=4).
            Returns:

            list[EmbeddingDocument]: List of processed documents.
        )doc");
}
// --------------------------------------------------------------------------
// Binding for Embedding::BaseEmbedding
// --------------------------------------------------------------------------

/**
 * Trampoline class (Python wrapper) for `BaseEmbedding`, as this class
 * still has a purely virtual method (`GenerateEmbeddings`). We need to
 * allow overriding of this method in Python.
 */

class PyBaseEmbedding : public Embedding::BaseEmbedding
{
public:
    using Embedding::BaseEmbedding::BaseEmbedding;

    std::vector<float> GenerateEmbeddings(const std::vector<std::string> &text) override
    {
        PYBIND11_OVERRIDE_PURE(
            std::vector<float>,
            Embedding::BaseEmbedding,
            GenerateEmbeddings,
            text);
    }

    Embedding::Document ProcessDocument(Embedding::Document document) override
    {
        PYBIND11_OVERRIDE(
            Embedding::Document,
            Embedding::BaseEmbedding,
            ProcessDocument,
            document);
    }

    // Overriding the virtual method ProcessDocuments (not pure)
    std::vector<Embedding::Document> ProcessDocuments(std::vector<Embedding::Document> documents, const int &maxWorkers) override
    {
        PYBIND11_OVERRIDE(
            std::vector<Embedding::Document>,
            Embedding::BaseEmbedding,
            ProcessDocuments,
            documents,
            maxWorkers);
    }
};

/**
 * Creates the binding for the `BaseEmbedding` class, which inherits from `IBaseEmbedding`.
 * The `BaseEmbedding` class still has a purely virtual method (`GenerateEmbeddings`),
 * so it cannot be instantiated directly.
 */

void bind_BaseEmbedding(py::module &m)
{
    py::class_<Embedding::BaseEmbedding, PyBaseEmbedding, std::shared_ptr<Embedding::BaseEmbedding>, Embedding::IBaseEmbedding>(
        m,
        "BaseEmbedding",
        R"doc(
            Abstract class that implements part of the embedding logic, inheriting
            from IBaseEmbedding. It still has a purely virtual method
            (GenerateEmbeddings), making it non-instantiable directly.
        )doc")
        .def(
            py::init<>(),
            R"doc(
            Default constructor for the BaseEmbedding class.
        )doc")
        .def(
            "GenerateEmbeddings",
            &Embedding::BaseEmbedding::GenerateEmbeddings,
            py::arg("text"),
            R"doc(
            Pure virtual method that generates embeddings for a set
            of strings. It must be overridden by concrete derived classes.
        )doc")
        .def(
            "ProcessDocument",
            &Embedding::BaseEmbedding::ProcessDocument,
            py::arg("document"),
            R"doc(
            Processes (generates embeddings) for a single document. It can be
            overridden in derived classes to alter its behavior.
        )doc")
        .def(
            "ProcessDocuments",
            &Embedding::BaseEmbedding::ProcessDocuments,
            py::arg("documents"),
            py::arg("maxWorkers") = 4,
            R"doc(
            Processes (generates embeddings) for multiple documents in parallel,
            using up to maxWorkers threads. It can be overridden in derived classes.
        )doc");
}
// --------------------------------------------------------------------------
// Binding for EmbeddingOpenAI::IEmbeddingOpenAI
// --------------------------------------------------------------------------

/**
 * Trampoline class (Python wrapper) for `IEmbeddingOpenAI`.
 * This interface inherits from `BaseEmbedding`, which also has virtual methods
 * (including a purely virtual one: `GenerateEmbeddings`).
 * Therefore, we need to override:
 *  - `SetAPIKey` (purely virtual in `IEmbeddingOpenAI`)
 *  - `GenerateEmbeddings` (purely virtual in `BaseEmbedding`)
 *  - `ProcessDocument` and `ProcessDocuments` (virtual in `BaseEmbedding`)
 */

class PyIEmbeddingOpenAI : public EmbeddingOpenAI::IEmbeddingOpenAI
{
public:
    // Uses constructor(s) from the base class
    using EmbeddingOpenAI::IEmbeddingOpenAI::IEmbeddingOpenAI;

    ~PyIEmbeddingOpenAI() override = default;

    // ----------------------------------------------------------------------
    // Pure methods of IEmbeddingOpenAI
    // ----------------------------------------------------------------------
    void SetAPIKey(const std::string &apiKey) override
    {
        PYBIND11_OVERRIDE_PURE(
            void,                              // TYpe of class
            EmbeddingOpenAI::IEmbeddingOpenAI, // Base class
            SetAPIKey,                         // Name of method
            apiKey                             // Parameter
        );
    }

    // ----------------------------------------------------------------------
    // Methods (pure or virtual) inherited from BaseEmbedding.
    // ----------------------------------------------------------------------
    std::vector<float> GenerateEmbeddings(const std::vector<std::string> &text) override
    {
        PYBIND11_OVERRIDE_PURE(
            std::vector<float>,                // Type of returns
            EmbeddingOpenAI::IEmbeddingOpenAI, // Base Class
            GenerateEmbeddings,                // Name of method
            text                               // Paraeter
        );
    }

    Embedding::Document ProcessDocument(Embedding::Document document) override
    {
        PYBIND11_OVERRIDE(
            Embedding::Document,
            EmbeddingOpenAI::IEmbeddingOpenAI,
            ProcessDocument,
            document);
    }

    std::vector<Embedding::Document> ProcessDocuments(std::vector<Embedding::Document> documents, const int &maxWorkers) override
    {
        PYBIND11_OVERRIDE(
            std::vector<Embedding::Document>,
            EmbeddingOpenAI::IEmbeddingOpenAI,
            ProcessDocuments,
            documents,
            maxWorkers);
    }
};

/**
 * Creates the binding for the `IEmbeddingOpenAI` interface.
 * Since it inherits from `BaseEmbedding` (which is also abstract) and
 * has pure virtual method(s), it cannot be instantiated directly in Python
 * without overriding these methods.
 */

void bind_IEmbeddingOpenAI(py::module &m)
{
    py::class_<EmbeddingOpenAI::IEmbeddingOpenAI,
               PyIEmbeddingOpenAI,
               EmbeddingOpenAI::IEmbeddingOpenAIPtr,
               Embedding::BaseEmbedding>(
        m,
        "IEmbeddingOpenAI",
        R"doc(
            Interface that inherits from Embedding::BaseEmbedding and adds the purely virtual method
            SetAPIKey for configuring the API key required to generate embeddings using OpenAI.

            Main methods:

            SetAPIKey(apiKey: str) -> None
            GenerateEmbeddings(text: list[str]) -> list[float]
            ProcessDocument(document: EmbeddingDocument) -> EmbeddingDocument
            ProcessDocuments(documents: list[EmbeddingDocument], maxWorkers: int) -> list[EmbeddingDocument]
        )doc")
        .def(
            py::init<>(),
            R"doc(
            Default constructor for the IEmbeddingOpenAI interface.
            Since it is an interface, it must be overridden in Python
            to instantiate a concrete object.
        )doc")
        .def(
            "SetAPIKey",
            &EmbeddingOpenAI::IEmbeddingOpenAI::SetAPIKey,
            py::arg("apiKey"),
            R"doc(
            Sets the API key to be used for generating embeddings  
            (in this case, the OpenAI API key).
        )doc")
        .def(
            "GenerateEmbeddings",
            &EmbeddingOpenAI::IEmbeddingOpenAI::GenerateEmbeddings,
            py::arg("text"),
            R"doc(
            Gera embeddings para uma lista de strings, usando 
            o modelo configurado (OpenAI).
        )doc")
        .def(
            "ProcessDocument",
            &EmbeddingOpenAI::IEmbeddingOpenAI::ProcessDocument,
            py::arg("document"),
            R"doc(
            Generates embeddings for a list of strings, using
            the configured model (OpenAI).
        )doc")
        .def(
            "ProcessDocuments",
            &EmbeddingOpenAI::IEmbeddingOpenAI::ProcessDocuments,
            py::arg("documents"),
            py::arg("maxWorkers") = 4,
            R"doc(
            Generates embeddings for multiple documents in parallel,
            using up to maxWorkers threads.
        )doc");
}
// --------------------------------------------------------------------------
// Binding for EmbeddingOpenAI::EmbeddingOpenAI.
// --------------------------------------------------------------------------

/**
    Creates the binding for the concrete class EmbeddingOpenAI, which inherits from
    IEmbeddingOpenAI (and therefore, from BaseEmbedding). This implementation
    uses the OpenAI API to generate embeddings.
 */
void bind_EmbeddingOpenAI(py::module &m)
{
    py::class_<EmbeddingOpenAI::EmbeddingOpenAI,
               std::shared_ptr<EmbeddingOpenAI::EmbeddingOpenAI>,
               EmbeddingOpenAI::IEmbeddingOpenAI>
        cls(
            m,
            "EmbeddingOpenAI",
            R"doc(
            Concrete class that implements IEmbeddingOpenAI, allowing the use
            of the OpenAI API for generating embeddings. Example of usage in Python:

            python
            Copy
            from RagPUREAI import EmbeddingOpenAI

            emb = EmbeddingOpenAI()
            emb.SetAPIKey("your_openai_key")
            embeddings = emb.GenerateEmbeddings(["example text", "more text"])
            Alternatively, you can also leverage the methods inherited
            from BaseEmbedding, such as .ProcessDocument() and .ProcessDocuments().
        )doc");

    cls.def(
           py::init<>(),
           R"doc(
            Default constructor for the EmbeddingOpenAI class.
        )doc")
        .def(
            "SetAPIKey",
            &EmbeddingOpenAI::EmbeddingOpenAI::SetAPIKey,
            py::arg("apiKey"),
            R"doc(
            Defines the OpenAI API key, required to use
            the embeddings endpoint. Internally, this key will be
            configured in the openai::start(apiKey) client.
        )doc")
        .def(
            "GenerateEmbeddings",
            &EmbeddingOpenAI::EmbeddingOpenAI::GenerateEmbeddings,
            py::arg("text"),
            R"doc(
            Generates embeddings for a list of strings, using the
            OpenAI model "text-embedding-ada-002". It may raise
            a RagException if an error occurs in the JSON response.

            Parameters:

            text (list[str]): List of input texts.
            Returns:

            list[float]: Vector with the concatenated embedding values.
        )doc");

    // If you wish to expose ProcessDocument and ProcessDocuments, which come from BaseEmbedding / IEmbeddingOpenAI,
    // you can add .def(...) calls here. Otherwise, they are inherited and already accessible on the Python side.
    // For example:
    cls.def(
        "ProcessDocument",
        &EmbeddingOpenAI::EmbeddingOpenAI::ProcessDocument,
        py::arg("document"),
        R"doc(
            Generates embeddings for the provided document, inserting the resulting vector
            into the embeddings field of the EmbeddingDocument.
        )doc");

    cls.def(
        "ProcessDocuments",
        &EmbeddingOpenAI::EmbeddingOpenAI::ProcessDocuments,
        py::arg("documents"),
        py::arg("maxWorkers") = 4,
        R"doc(
            Generates embeddings for multiple documents in parallel,
            using up to maxWorkers threads.
        )doc");
}

//--------------------------------------------------------------------------
//  Binding function for ChunkQuery
//--------------------------------------------------------------------------
void bind_ChunkQuery(py::module &m)
{
    //--------------------------------------------------------------------------
    // Binding for the Chunk::ChunkQuery class.
    //--------------------------------------------------------------------------

    py::class_<Chunk::ChunkQuery, std::shared_ptr<Chunk::ChunkQuery>>(m, "ChunkQuery", R"doc(
        Class for processing queries in text chunks, generating embeddings, and
        evaluating similarity with a provided query.
    )doc")
        .def(py::init<int, int, Chunk::EmbeddingModel, const std::string &>(),
             py::arg("chunk_size") = 100,
             py::arg("overlap") = 20,
             py::arg("embedding_model") = Chunk::EmbeddingModel::HuggingFace,
             py::arg("openai_api_key") = "",
             R"doc(
                 Constructor for the ChunkQuery class.

                Parameters:

                chunk_size (int, optional): Size of each text chunk (default=100).
                overlap (int, optional): Overlap between successive chunks (default=20).
                embedding_model (EmbeddingModel, optional): Embedding model to be used (HuggingFace or OpenAI).
                openai_api_key (str, optional): OpenAI API key (required if embedding_model=OpenAI).
             )doc")

        // Method: ProcessSingleDocument
        .def("ProcessSingleDocument",
             &Chunk::ChunkQuery::ProcessSingleDocument,
             py::arg("item"),
             py::arg("query_embedding"),
             py::arg("similarity_threshold"),
             R"doc(
                 Processes a single document by splitting it into chunks, generating embeddings,
                and evaluating similarity with the provided query.

                Parameters:

                item (RAGLibrary.LoaderDataStruct): Structure containing the file identifier and textual content.
                query_embedding (list[float]): Embedding of the query for comparison.
                similarity_threshold (float): Similarity threshold for considering a chunk relevant.
                Returns:

                list[RAGLibrary.Document]: List of documents that meet the similarity criteria.
             )doc")

        // Method: ProcessDocuments
        .def("ProcessDocuments",
             &Chunk::ChunkQuery::ProcessDocuments,
             py::arg("items"),
             py::arg("query"),
             py::arg("similarity_threshold"),
             py::arg("max_workers") = 4,
             R"doc(
                 Processes multiple documents in parallel, splitting each into chunks,
                generating embeddings, and evaluating similarity with the provided query.

                Parameters:

                items (list[RAGLibrary.LoaderDataStruct]): List of structures containing identifiers and textual contents.
                query (str): Query text to generate embeddings and compare.
                similarity_threshold (float): Similarity threshold for considering a chunk relevant.
                max_workers (int, optional): Maximum number of threads to be used in parallel processing (default=4).
                Returns:

                list[RAGLibrary.Document]: List of documents that meet the similarity criteria.
             )doc");
}

//--------------------------------------------------------------------------
// Main module
//--------------------------------------------------------------------------
PYBIND11_MODULE(RagPUREAI, m)
{
    m.doc() = "Unified bindings for RagPUREAI.";
    bind_RagException(m);
    bind_FileUtilsLocal(m);
    bind_StringUtils(m);

    bind_CommonStructs(m);

    bind_IBaseDataLoader(m);
    bind_BaseDataLoader(m);
    bind_PDFLoader(m);
    bind_DOCXLoader(m);
    bind_TXTLoader(m);
    bind_WebLoader(m);

    bind_Document(m);
    bind_IMetadataExtractor(m);
    bind_MetadataExtractor(m);
    bind_IMetadataHFExtractor(m);
    bind_IMetadataRegexExtractor(m);
    bind_MetadataRegexExtractor(m);
    bind_MetadataHFExtractor(m);

    // bind_EmbeddingModel(m);
    bind_ChunkCommons(m);
    bind_ContentCleaner(m);
    bind_ChunkDefault(m);
    bind_ChunkCount(m);
    bind_ChunkQuery(m);
    bind_ChunkSimilarity(m);
    bind_EmbeddingDocument(m);
    bind_IBaseEmbedding(m);
    bind_BaseEmbedding(m);
    bind_IEmbeddingOpenAI(m);
    bind_EmbeddingOpenAI(m);
}
