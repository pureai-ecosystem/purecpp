
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <pybind11/complex.h>
#include <pybind11/chrono.h>
#include <pybind11/numpy.h>
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

#include "EmbeddingOpenAI/IEmbeddingOpenAI.h"
#include "EmbeddingOpenAI/EmbeddingOpenAI.h"

namespace py = pybind11;
using namespace RAGLibrary;
using namespace DataLoader;

typedef std::vector<std::pair<std::string, std::string>> test_vec_pair;
// using test_vec_pair as std::vector<std::pair<std::string, std::string>>;

// Do not use using namespace PDFLoader; to avoid conflicts.
// The class will be referenced using full qualification.
template <typename Type>
void bindThreadSafeQueue2(py::module& m, const std::string& name)
{
    py::class_<ThreadSafeQueue<Type>>(m, name.c_str())
        .def(py::init<>())
        .def(py::init<const ThreadSafeQueue<Type> &>())
        .def(py::init<const std::vector<Type> &>(), py::arg("vect"))
        .def("push", &ThreadSafeQueue<Type>::push, py::arg("value"))
        .def("pop", &ThreadSafeQueue<Type>::pop)
        .def("size", &ThreadSafeQueue<Type>::size);
}
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

void bind_CommonStructs(py::module& m)
{
    // ----------------------------------------------------------------------
    // DataExtractRequestStruct
    // ----------------------------------------------------------------------
        py::class_<RAGLibrary::LoaderDataStruct>(m, "LoaderDataStruct")
        .def(py::init<const RAGLibrary::Metadata&, const std::vector<std::string> &>(),
            py::arg("metadata"), py::arg("textContent"))
        .def_readwrite("metadata", &RAGLibrary::LoaderDataStruct::metadata)
        .def_readwrite("textContent", &RAGLibrary::LoaderDataStruct::textContent)
        .def("__str__", [](const RAGLibrary::LoaderDataStruct& data)
            {  std::ostringstream o;
                o << data; 
                return o.str(); });

    py::class_<RAGLibrary::DataExtractRequestStruct>(m, "DataExtractRequestStruct")
        .def(py::init<>())
        .def(py::init<const std::string&, unsigned int>(),
            py::arg("targetIdentifier"), py::arg("extractContentLimit") = 0)
        .def_readwrite("targetIdentifier", &RAGLibrary::DataExtractRequestStruct::targetIdentifier)
        .def_readwrite("extractContentLimit", &RAGLibrary::DataExtractRequestStruct::extractContentLimit);
 
 
    // ----------------------------------------------------------------------
    // ThreadSafeQueue<DataExtractRequestStruct> and ThreadSafeQueue<std::string>
    // ----------------------------------------------------------------------
    bindThreadSafeQueue<RAGLibrary::DataExtractRequestStruct>(m, "ThreadSafeQueueDataRequest");
    bindThreadSafeQueue<std::string>(m, "ThreadSafeQueueString");
 
    // ----------------------------------------------------------------------
    // ThreadStruct
    // ----------------------------------------------------------------------
    py::class_<RAGLibrary::ThreadStruct>(m, "ThreadStruct",
        R"doc(
            Structure that represents thread of work:
              - threadRunner: a std::future<void> managed by a shared_ptr.
              - threadQueue: a queue (ThreadSafeQueueDataRequest).
              - threadRemainingWork: Shows the remaining work
        )doc")
        .def(py::init<>(),
            R"doc(
                 Default constructor.
             )doc")
        .def(py::init<
            std::shared_ptr<std::future<void>>,
            RAGLibrary::ThreadSafeQueueDataRequest,
            unsigned int>(),
            py::arg("threadRunner"),
            py::arg("threadQueue"),
            py::arg("threadRemainingWork"),
            R"doc(
                 Receives std::future thread(threadRunner),
                 data queue(threadQueue) and amount of work
                 remaining (threadRemainingWork).
             )doc")
        .def_readwrite("threadRunner", &RAGLibrary::ThreadStruct::threadRunner,
            "std::shared_ptr<std::future<void>> to sync end of thread.")
        .def_readwrite("threadQueue", &RAGLibrary::ThreadStruct::threadQueue,
            "Thread queue.")
        .def_readwrite("threadRemainingWork", &RAGLibrary::ThreadStruct::threadRemainingWork,
            "Amount of remaining work");
 
    // ----------------------------------------------------------------------
    // KeywordData
    // ----------------------------------------------------------------------
    py::class_<RAGLibrary::KeywordData>(m, "KeywordData",
        R"doc(
            Structure that store key-value occurences:
              - occurences: all occurences
              - position: vector of pairs (line, offset)
        )doc")
        .def(py::init<>(),
            R"doc(
                 Default Constructor, initialize occurences = 0 and void position.
             )doc")
        .def_readwrite("occurrences", &RAGLibrary::KeywordData::occurrences,
            "Total number of occurrences found.")
        .def_readwrite("position", &RAGLibrary::KeywordData::position,
            R"doc(
                 Positions of each occurrence as (line, offset) pairs.
             )doc");
 
    // ----------------------------------------------------------------------
    // UpperKeywordData
    // ----------------------------------------------------------------------
    py::class_<RAGLibrary::UpperKeywordData>(m, "UpperKeywordData",
        R"doc(
            Structure that aggregates keyword occurrences across multiple files,
            including:
              - totalOccurrences: overall total
              - keywordDataPerFile: map<string, KeywordData> (per file)
        )doc")
        .def(py::init<>(),
            R"doc(
                 Default constructor, initializes totalOccurrences = 0.
             )doc")
        .def_readwrite("totalOccurences", &RAGLibrary::UpperKeywordData::totalOccurences,
            "Total occurrences of the keyword across all files.")
        .def_readwrite("keywordDataPerFile", &RAGLibrary::UpperKeywordData::keywordDataPerFile,
            R"doc(
                 Maps the filename (string) to keyword data (KeywordData).
             )doc")
        .def("__str__", [](const RAGLibrary::UpperKeywordData& data)
            {
                std::ostringstream oss;
                oss << data;
                return oss.str(); });
 
    py::class_<RAGLibrary::Document>(m, "RAGDocument")
        .def(py::init<>())
        .def(py::init<RAGLibrary::Metadata, const std::string &>(),
             py::arg("metadata"), py::arg("page_content"))
        .def_readwrite("metadata", &RAGLibrary::Document::metadata)
        .def_readwrite("page_content", &RAGLibrary::Document::page_content)
        .def_readwrite("embedding", &RAGLibrary::Document::embedding)
        .def("StringRepr", &RAGLibrary::Document::StringRepr)
        .def("__repr__", [](const RAGLibrary::Document &doc)
             {
                std::ostringstream o;
                o << doc;
                return o.str(); })
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
    std::vector<RAGLibrary::Document> Load() override
    {
        PYBIND11_OVERRIDE_PURE(
            std::vector<RAGLibrary::Document>,
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
        .def("Load", &BaseDataLoader::Load)
        .def("KeywordExists", &BaseDataLoader::KeywordExists, py::arg("pdfFileName"), py::arg("keyword"))
        .def("GetKeywordOccurences", &BaseDataLoader::GetKeywordOccurences, py::arg("keyword"));
}

//--------------------------------------------------------------------------
// Binding for ContentCleaner.
//--------------------------------------------------------------------------
typedef std::vector<std::pair<std::string, std::string>> test_vec_pair;
void bind_ChunkCommons(py::module& m)
{
    //--------------------------------------------------------------------------
    // Binding enum  for EmbeddingModel
    //--------------------------------------------------------------------------
    py::dict model_dict;
    for (const auto& [vendor, models] : Chunk::EmbeddingModel) {
        py::list model_list;
        for (const auto& m : models) model_list.append(m);
        model_dict[vendor.c_str()] = model_list;
    }
    m.attr("embedding_model_map") = model_dict;
    
    m.def("resolve_vendor_from_model", &Chunk::resolve_vendor_from_model);
    m.def("resolve_vendor", &Chunk::resolve_vendor);
    m.def("to_lowercase", &Chunk::to_lowercase);
    py::class_<Chunk::vdb_data>(m, "VDBdata", R"doc(
            Represents an entry in the Vector DataBase.

            Attributes:
                flatVD (List[float]): Flat vector of embeddings.
                vendor (str): Vendor used.
                model (str): Model name.
                dim (int): Embedding dimension.
                n (int): Number of chunks.
        )doc")
    .def(py::init<>())
    .def_readwrite("flatVD", &Chunk::vdb_data::flatVD)
    .def_readwrite("vendor", &Chunk::vdb_data::vendor)
    .def_readwrite("model", &Chunk::vdb_data::model)
    .def_readwrite("dim", &Chunk::vdb_data::dim)
    .def_readwrite("n", &Chunk::vdb_data::n);


    //--------------------------------------------------------------------------
    // Binding function MeanPooling
    //--------------------------------------------------------------------------
    m.def("MeanPooling", &Chunk::MeanPooling,
        py::arg("token_embeddings"), py::arg("attention_mask"), py::arg("embedding_size"),
        R"doc(
               Calculates the average of embeddings based on the attention mask.

               Parameters:
                   token_embeddings (list[float]): List of token embeddings.
                   attention_mask (list[int]): List of attention masks (1 or 0).
                   embedding_size (int): Size of the embeddings.

               Returns:
                   list[float]: List of mean embeddings.
           )doc");
 
    //--------------------------------------------------------------------------
    // Binding function NormalizeEmbeddings
    //--------------------------------------------------------------------------
    m.def("NormalizeEmbeddings", &Chunk::NormalizeEmbeddings,
        py::arg("embeddings"),
        R"doc(
               Normalizes the embeddings to have a norm of 1.

               Parameters:
                   embeddings (list[float]): List of embeddings to be normalized.
           )doc");
 
    //--------------------------------------------------------------------------
    // Binding function for EmbeddingModelBatch
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
    // Binding function for EmbeddingHuggingFaceTransformers
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
    // Binding function for EmbeddingOpeanAI
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
    // Binding function for toTensor
    //--------------------------------------------------------------------------
    m.def("toTensor", &Chunk::toTensor,
        py::arg("vect"),
        R"doc(
               Converts a list of lists of floats into a PyTorch tensor.

               Parameters:
                   vect (list[list[float]]): List of lists of floats.

               Returns:
                   torch.Tensor: A PyTorch tensor.
           )doc");
 
    //--------------------------------------------------------------------------
    // Binding function for SplitText
    //--------------------------------------------------------------------------
    m.def("SplitText", &Chunk::SplitText,
        py::arg("inputs"), py::arg("overlap"), py::arg("chunk_size"),
        R"doc(
               Splits text into chunks with overlap.

               Parameters:
                   inputs (list[str]): List of input strings.
                   overlap (int): Number of characters for overlap.
                   chunk_size (int): Size of each chunk.

               Returns:
                   list[str]: List of resulting chunks.
           )doc");
 
    //--------------------------------------------------------------------------
    // Binding for the function SplitTextByCount
    //--------------------------------------------------------------------------
    m.def("SplitTextByCount", &Chunk::SplitTextByCount,
        py::arg("inputs"), py::arg("overlap"), py::arg("count_threshold"), py::arg("regex"),
        R"doc(
               Splits text into chunks based on a count and a regex.

               Parameters:
                   inputs (list[str]): List of input strings.
                   overlap (int): Number of characters for overlap.
                   count_threshold (int): Count limit for splitting.
                   regex (re2.RE2): Regular expression to identify split points.

               Returns:
                   list[str]: List of resulting chunks.
           )doc");
}
 
//--------------------------------------------------------------------------
// Binding for ChunkDefault
//--------------------------------------------------------------------------
void bind_ChunkDefault(py::module& m)
{
    py::class_<Chunk::ChunkDefault>(m, "ChunkDefault")
        .def(py::init<int, int, std::optional<std::vector<RAGLibrary::Document>>, int>(),
             py::arg("chunk_size") = 100,
             py::arg("overlap") = 20,
             py::arg("items_opt") = std::nullopt,
             py::arg("max_workers") = 4)

        .def("ProcessDocuments", &Chunk::ChunkDefault::ProcessDocuments,
             py::arg("items_opt") = std::nullopt,
             py::arg("max_workers") = 4,
             "Processes a list of documents into chunks.")

        .def("CreateEmb", &Chunk::ChunkDefault::CreateEmb,
             py::arg("model") = "text-embedding-ada-002",
             py::return_value_policy::reference,
             "Creates and stores embeddings for the current chunks.")

        .def("getflatVD", [](const Chunk::ChunkDefault &self, size_t idx) {
            const auto &vec = self.getFlatVD(idx);
            const auto *elem = self.getElement(idx);
            if (!elem) throw std::out_of_range("Invalid index for get_flat_vd");

            size_t n = elem->n;
            size_t dim = elem->dim;
            if (vec.size() != n * dim) throw std::runtime_error("Inconsistency in the flattened vector.");

            return py::array_t<float>(
                {n, dim},
                {sizeof(float) * dim, sizeof(float)},
                vec.data(),
                py::cast(self)
            );
        }, py::arg("idx"),
        "Returns the flattened vector as a numpy array [n, dim].")

        .def("printVD", &Chunk::ChunkDefault::printVD)
        .def("clear", &Chunk::ChunkDefault::clear)
        .def("isInitialized", &Chunk::ChunkDefault::isInitialized)
        .def("quant_of_elements", &Chunk::ChunkDefault::quant_of_elements)
        .def("getChunks", &Chunk::ChunkDefault::getChunks, py::return_value_policy::reference);
}
 
//--------------------------------------------------------------------------
// Binding for ChunkCount
//--------------------------------------------------------------------------
void bind_ChunkCount(py::module& m)
{
    py::class_<Chunk::ChunkCount>(m, "ChunkCount")
        .def(py::init<const std::string&, const int, const int>(),
            py::arg("count_unit"), py::arg("overlap") = 600, py::arg("count_threshold") = 1)
        .def(py::init<>()) // Default constructor also exists
 
        .def("ProcessSingleDocument", &Chunk::ChunkCount::ProcessSingleDocument, py::arg("item"))
 
        .def("ProcessDocuments", &Chunk::ChunkCount::ProcessDocuments,
            py::arg("items"), py::arg("max_workers") = 4);
}
 
// --------------------------------------------------------------------------
// Binding for Chunk::ChunkSimilarity
// --------------------------------------------------------------------------
 
void bind_ChunkSimilarity(py::module& m)
{
 
    py::class_<Chunk::ChunkSimilarity>(m, "ChunkSimilarity", R"doc(
        Class to process RAGDocuments and generate chunks and embeddings,
        allowing for document similarity assessment. It has options to
        define chunk size, overlap, embedding model (HuggingFace or OpenAI),
        and an API key for OpenAI if needed.
    )doc")
        .def(
            py::init<int, int, std::string, const std::string&>(),
            py::arg("chunk_size") = 100,
            py::arg("overlap") = 20,
            py::arg("embedding_model") = "openai",
            py::arg("openai_api_key") = "",
            R"doc(
                Constructor that initializes the ChunkSimilarity class.

                Parameters:
                    chunk_size (int): Default size of each text chunk (default=100).
                    overlap (int): Overlap between successive chunks (default=20).
                    embedding_model (EmbeddingModel): Embedding model (HuggingFace or OpenAI).
                    openai_api_key (str): OpenAI API key (only if embedding_model=OpenAI).
            )doc")
        .def(
            "ProcessSingleDocument",
            &Chunk::ChunkSimilarity::ProcessSingleDocument,
            py::arg("item"),
            R"doc(
                Given a single RAGDocument, splits its content into chunks
                and generates embeddings according to the chosen model, returning
                a vector of RAGLibrary::Document.

                Parameters:
                    item (RAGDocument): Structure containing
                        the identifier and the textual content.

                Returns:
                    list[RAGDocument]: Vector of resulting documents,
                    each with its chunked content and possibly
                    with associated embeddings in its metadata.
            )doc")
        .def(
            "ProcessDocuments",
            &Chunk::ChunkSimilarity::ProcessDocuments,
            py::arg("items"),
            py::arg("max_workers") = 4,
            R"doc(
                Similar to ProcessSingleDocument, but processes multiple
                RAGDocuments in parallel (up to max_workers).

                Parameters:
                    items (list[RAGDocument]): List of
                        structures containing the content to be chunked.
                    max_workers (int): Maximum number of threads to use
                        in parallel processing (default=4).

                Returns:
                    list[RAGDocument]: Concatenated list of documents
                    resulting from the chunking of each item.
            )doc");
}
//--------------------------------------------------------------------------
// Binding function for ChunkQuery
//--------------------------------------------------------------------------

void bind_ChunkQuery(py::module_& m) {
    py::class_<Chunk::ChunkQuery>(m, "ChunkQuery")
        .def(py::init<
            std::string,
            RAGLibrary::Document,
            const Chunk::ChunkDefault*,
            std::optional<size_t>,
            float>(),
            py::arg("query") = "",
            py::arg("query_doc") = RAGLibrary::Document(),
            py::arg("chunks") = nullptr,
            py::arg("pos") = std::nullopt,
            py::arg("threshold") = -5
        )

        .def("Query", 
            py::overload_cast<
                std::string,
                const Chunk::ChunkDefault*,
                std::optional<size_t>
            >(&Chunk::ChunkQuery::Query),
            py::arg("query"),
            py::arg("chunks") = nullptr,
            py::arg("pos") = std::nullopt
        )

        .def("Query", 
            py::overload_cast<
                RAGLibrary::Document,
                const Chunk::ChunkDefault*,
                std::optional<size_t>
            >(&Chunk::ChunkQuery::Query),
            py::arg("query_doc"),
            py::arg("chunks") = nullptr,
            py::arg("pos") = std::nullopt
        )

        .def("Retrieve", &Chunk::ChunkQuery::Retrieve,
            py::arg("threshold") = 0.5f,
            py::arg("chunks") = nullptr,
            py::arg("pos") = std::nullopt
        )

        .def("getQuery", &Chunk::ChunkQuery::getQuery)
        .def("getMod", &Chunk::ChunkQuery::getMod)
        .def("getPar", &Chunk::ChunkQuery::getPar)
        .def("getChunksList", &Chunk::ChunkQuery::getChunksList,
            py::return_value_policy::reference)

        .def("getRetrieveList", &Chunk::ChunkQuery::getRetrieveList)
        .def("strQ", &Chunk::ChunkQuery::StrQ, py::arg("index") = -1)

        .def("setChunks", &Chunk::ChunkQuery::setChunks,
        py::arg("chunks"),
        py::arg("pos") = 0,
        "Configures the chunk structure and prepares spans for retrieval")

        ;
}

//--------------------------------------------------------------------------
// Binding for ContentCleaner
//--------------------------------------------------------------------------
void bind_ContentCleaner(py::module& m)
{
    py::class_<CleanData::ContentCleaner>(m, "ContentCleaner")
        .def(py::init<const std::vector<std::string> &>(), py::arg("default_patterns") = std::vector<std::string>{})
        .def("ProcessDocument", &CleanData::ContentCleaner::ProcessDocument,
            py::arg("doc"), py::arg("custom_patterns") = std::vector<std::string>{})
        .def("ProcessDocuments", &CleanData::ContentCleaner::ProcessDocuments,
            py::arg("docs"), py::arg("custom_patterns") = std::vector<std::string>{}, py::arg("max_workers") = 4);
}


//--------------------------------------------------------------------------
// Binding for PDFLoader
//--------------------------------------------------------------------------
void bind_PDFLoader(py::module& m)
{
    py::class_<::PDFLoader::PDFLoader, std::shared_ptr<::PDFLoader::PDFLoader>, DataLoader::BaseDataLoader>(m, "PDFLoader")
        .def(py::init<const std::string, const unsigned int &>(),
            py::arg("filePath"),
            py::arg("numThreads") = 1,
            "Creates a PDFLoader with a file path and an optional number of threads.");
}
void bind_DOCXLoader(py::module& m)
{
    py::class_<::DOCXLoader::DOCXLoader, std::shared_ptr<::DOCXLoader::DOCXLoader>, DataLoader::BaseDataLoader>(m, "DOCXLoader")
        .def(py::init<const std::string, const unsigned int &>(),
            py::arg("filePath"),
            py::arg("numThreads") = 1,
            "Creates a DOCXLoader with a file path and an optional number of threads.");
}
 
void bind_TXTLoader(py::module& m)
{
    py::class_<::TXTLoader::TXTLoader, std::shared_ptr<::TXTLoader::TXTLoader>, DataLoader::BaseDataLoader>(m, "TXTLoader")
        .def(py::init<const std::string, const unsigned int &>(),
            py::arg("filePath"),
            py::arg("numThreads") = 1,
            "Creates a TXTLoader, optionally with initial paths and a defined number of threads.");
}
 
void bind_WebLoader(py::module& m)
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
void bind_Document(py::module& m)
{
    py::class_<::MetadataExtractor::Document>(m, "Document")
        .def(
            py::init<
            const std::vector<std::string> &,
            const std::vector<std::pair<std::string, std::string>> &>(),
            py::arg("pageContent"),
            py::arg("metadata") = std::vector<std::pair<std::string, std::string>>{},
            R"doc(
                Document constructor.

                Parameters:
                    pageContent (list[str]): list of strings representing the document content.
                    metadata (list[tuple[str, str]]): list of key-value pairs for the metadata.
            )doc")
        .def_readwrite("pageContent", &::MetadataExtractor::Document::pageContent, R"doc(
                Field that stores the document content as a vector of strings.
            )doc")
        .def_readwrite("metadata", &::MetadataExtractor::Document::metadata, R"doc(
                Document metadata, stored as key-value pairs.
            )doc")
        .def("StringRepr", &::MetadataExtractor::Document::StringRepr,
            R"doc(
                Returns a string representation that lists each metadata item contained in the document.
            )doc");

    bindThreadSafeQueue2<::MetadataExtractor::Document>(m, "ThreadSafeQueueDocument");
}

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

    std::vector<::MetadataExtractor::Document> ProcessDocuments(std::vector<::MetadataExtractor::Document> docs, const int& maxWorkers) override
    {
        PYBIND11_OVERRIDE_PURE(
            std::vector<::MetadataExtractor::Document>,
            MetadataExtractor::IMetadataExtractor,
            ProcessDocuments,
            docs, maxWorkers);
    }
};

void bind_IMetadataExtractor(py::module& m)
{
    py::class_<MetadataExtractor::IMetadataExtractor, PyIMetadataExtractor, MetadataExtractor::IMetadataExtractorPtr>(m, "IMetadataExtractor")
        .def(py::init<>())
        .def("ProcessDocument", &MetadataExtractor::IMetadataExtractor::ProcessDocument, py::arg("doc"))
        .def("ProcessDocuments", &MetadataExtractor::IMetadataExtractor::ProcessDocuments, py::arg("docs"), py::arg("maxWorkers"));
}
// --------------------------------------------------------------------------
// Binding for MetadataExtractor::MetadataExtractor
// --------------------------------------------------------------------------
class PyMetadataExtractor : public MetadataExtractor::MetadataExtractor
{
public:
    using MetadataExtractor::MetadataExtractor::MetadataExtractor;

    ::MetadataExtractor::Document ProcessDocument(::MetadataExtractor::Document doc) override
    {
        PYBIND11_OVERRIDE_PURE(
            ::MetadataExtractor::Document,          // Return type
            ::MetadataExtractor::MetadataExtractor, // Parent class
            ProcessDocument,                        // Function name
            doc                                     // Param 
        );
    }

    std::vector<::MetadataExtractor::Document> ProcessDocuments(std::vector<::MetadataExtractor::Document> docs, const int& maxWorkers) override
    {
        PYBIND11_OVERRIDE(
            std::vector<::MetadataExtractor::Document>, // Return type
            ::MetadataExtractor::MetadataExtractor,     // Parent class
            ProcessDocuments,                           //Fuction name 
            docs,                                       // param 1
            maxWorkers                                  // param 2
        );
    }
};

void bind_MetadataExtractor(py::module& m)
{
    py::class_<MetadataExtractor::MetadataExtractor, PyMetadataExtractor, std::shared_ptr<MetadataExtractor::MetadataExtractor>, MetadataExtractor::IMetadataExtractor>(
        m,
        "MetadataExtractor",
        R"doc(
            Base class for metadata extraction, inheriting from IMetadataExtractor.
            It has a pure virtual method for processing a single document, and
            a method for processing multiple documents in parallel.
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
            Processes metadata from a single document.
            This method is pure virtual and must be overridden
            in concrete derived classes.
        )doc")
        .def(
            "ProcessDocuments",
            &MetadataExtractor::MetadataExtractor::ProcessDocuments,
            py::arg("docs"),
            py::arg("maxWorkers") = 4,
            R"doc(
            Processes metadata from multiple documents, with support for parallelism.
            By default, it uses up to 4 threads (maxWorkers=4).
        )doc");
}

class PyIMetadataRegexExtractor : public MetadataRegexExtractor::IMetadataRegexExtractor
{
public:
    void AddPattern(const std::string& name, const std::string& pattern) override
    {
        PYBIND11_OVERRIDE_PURE(
            void,
            MetadataRegexExtractor::IMetadataRegexExtractor,
            AddPattern,
            name, pattern);
    }

    ::MetadataExtractor::Document ProcessDocument(::MetadataExtractor::Document doc) override
    {
        PYBIND11_OVERRIDE_PURE(
            ::MetadataExtractor::Document,
            MetadataRegexExtractor::IMetadataRegexExtractor,
            ProcessDocument,
            doc);
    }

    std::vector<::MetadataExtractor::Document> ProcessDocuments(std::vector<::MetadataExtractor::Document> docs, const int& maxWorkers) override
    {
        PYBIND11_OVERRIDE_PURE(
            std::vector<::MetadataExtractor::Document>,
            MetadataRegexExtractor::IMetadataRegexExtractor,
            ProcessDocuments,
            docs, maxWorkers);
    }
};

void bind_IMetadataRegexExtractor(py::module& m)
{
    py::class_<MetadataRegexExtractor::IMetadataRegexExtractor, PyIMetadataRegexExtractor, MetadataRegexExtractor::IMetadataRegexExtractorPtr, MetadataExtractor::IMetadataExtractor>(m, "IMetadataRegexExtractor")
        .def(py::init<>())
        .def("AddPattern", &MetadataRegexExtractor::IMetadataRegexExtractor::AddPattern, py::arg("name"), py::arg("pattern"))
        .def("ProcessDocument", &MetadataRegexExtractor::IMetadataRegexExtractor::ProcessDocument, py::arg("doc"))
        .def("ProcessDocuments", &MetadataRegexExtractor::IMetadataRegexExtractor::ProcessDocuments, py::arg("docs"), py::arg("maxWorkers"));
}

// --------------------------------------------------------------------------
// Binding for MetadataHFExtractor::IMetadataHFExtractor
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

    test_vec_pair ExtractMetadata(const std::vector<std::string>& text) override
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


void bind_IMetadataHFExtractor(py::module& m)
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
            Pure method to extract metadata (named entities) from one or more
            texts. Returns a vector of (token, entity) pairs.
        )doc")
        .def(
            "ProcessDocument",
            &MetadataHFExtractor::IMetadataHFExtractor::ProcessDocument,
            py::arg("doc"),
            R"doc(
            Pure method that processes metadata in a document of type 
            MetadataExtractor::Document. May include NER logic.
        )doc");
}

void bind_MetadataRegexExtractor(py::module& m)
{
    py::class_<MetadataRegexExtractor::MetadataRegexExtractor, std::shared_ptr<MetadataRegexExtractor::MetadataRegexExtractor>, MetadataRegexExtractor::IMetadataRegexExtractor>(m, "MetadataRegexExtractor")
        .def(py::init<>())
        .def("AddPattern", &MetadataRegexExtractor::MetadataRegexExtractor::AddPattern, py::arg("name"), py::arg("pattern"))
        .def("ProcessDocument", &MetadataRegexExtractor::MetadataRegexExtractor::ProcessDocument, py::arg("doc"))
        .def("ProcessDocuments", &MetadataRegexExtractor::MetadataRegexExtractor::ProcessDocuments, py::arg("docs"), py::arg("maxWorkers"));
}

// --------------------------------------------------------------------------
// Binding for MetadataHFExtractor::MetadataHFExtractor
// --------------------------------------------------------------------------

void bind_MetadataHFExtractor(py::module& m)
{
    py::class_<MetadataHFExtractor::MetadataHFExtractor,
        std::shared_ptr<MetadataHFExtractor::MetadataHFExtractor>,
        MetadataHFExtractor::IMetadataHFExtractor>(
            m,
            "MetadataHFExtractor",
            R"doc(
            Concrete class that implements metadata extraction using NER models
            via ONNXRuntime and tokenization libraries (HuggingFace tokenizers).
        )doc")
        .def(
            py::init<>(),
            R"doc(
            Default constructor that configures the ONNXRuntime environment, sessions, and 
            tokenizers needed for metadata extraction.
        )doc")
        .def(
            "InitializeNERModel",
            &MetadataHFExtractor::MetadataHFExtractor::InitializeNERModel,
            R"doc(
            Initializes the NER model by loading the ONNX file and preparing
            the inference environment (ONNXRuntime session, CPU provider, etc.).
        )doc")
        .def(
            "ExtractMetadata",
            &MetadataHFExtractor::MetadataHFExtractor::ExtractMetadata,
            py::arg("text"),
            R"doc(
            Executes metadata extraction (named entities) on one or more
            provided texts. Returns a vector of (entity, label) pairs.
            
            Parameters:
                text (list[str]): List of strings to be processed.

            Returns:
                list[tuple[str, str]]: Each element is a (token, entity) pair.
        )doc")
        .def(
            "ProcessDocument",
            &MetadataHFExtractor::MetadataHFExtractor::ProcessDocument,
            py::arg("doc"),
            R"doc(
            Processes metadata in a `MetadataExtractor.Document` object, including
            named entity detection. Returns the same `Document` object, but
            with updated metadata.

            Parameters:
                doc (MetadataExtractor.Document): Document to be processed.

            Returns:
                MetadataExtractor.Document: Document with named entities
                added to its metadata set.
        )doc");
}
// --------------------------------------------------------------------------
// Binding for Embedding::Document
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
            Main constructor that receives:

              - pageContent (list[str]): List of content.
              - metadata (list[tuple[str, str]]): List of key-value pairs as metadata.
              - embeddings (list[float]): Embeddings (vector of floats).
        )doc")
        .def(
            py::init<const ::MetadataExtractor::Document &>(),
            py::arg("document"),
            R"doc(
            Constructor that converts a 'MetadataExtractor::Document'
            into an 'EmbeddingDocument'.
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

    py::class_<Embedding::ThreadSafeQueueDocument>(
        m,
        "ThreadSafeQueueEmbeddingDocument",
        R"doc(
            Thread-safe queue of EmbeddingDocument, allowing concurrent access
            in parallel scenarios.
        )doc")
        .def(py::init<>())
        .def("push", &Embedding::ThreadSafeQueueDocument::push, py::arg("value"))
        .def("pop", &Embedding::ThreadSafeQueueDocument::pop)
        .def("size", &Embedding::ThreadSafeQueueDocument::size);
}
// --------------------------------------------------------------------------
// Binding for Embedding::IBaseEmbedding
// --------------------------------------------------------------------------

class PyIBaseEmbedding : public Embedding::IBaseEmbedding
{
public:
    ~PyIBaseEmbedding() = default;

    std::vector<RAGLibrary::Document> GenerateEmbeddings(const std::vector<RAGLibrary::Document> &documents, const std::string &model, size_t batch_size) override
    {
        PYBIND11_OVERRIDE_PURE(
            std::vector<RAGLibrary::Document>,
            Embedding::IBaseEmbedding,
            GenerateEmbeddings,
            documents,
            model,
            batch_size
        );
    }
};

void bind_IBaseEmbedding(py::module &m)
{
    py::class_<Embedding::IBaseEmbedding, PyIBaseEmbedding, Embedding::IBaseEmbeddingPtr>(
        m,
        "IBaseEmbedding",
        R"doc(
            Base interface for generating embeddings for texts or documents.
            It has methods for generating embeddings, processing a single document,
            and processing multiple documents in parallel.
        )doc")
        .def(
            py::init<>(),
            R"doc(
            Default constructor for the IBaseEmbedding interface. 
            Cannot be instantiated without overriding methods in Python.
        )doc")
        .def(
            "GenerateEmbeddings",
            &Embedding::IBaseEmbedding::GenerateEmbeddings,
            py::arg("documents"),
            py::arg("model"),
            py::arg("batch_size") = 32,
            R"doc(
            Generates embeddings for a vector of strings.
            
            Parameters:
                text (list[str]): list of texts to be converted into embeddings
            
            Returns:
                list[float]: vector of floats representing the concatenated embeddings.
        )doc");
}

// --------------------------------------------------------------------------
// Binding para EmbeddingOpenAI::IEmbeddingOpenAI
// --------------------------------------------------------------------------

/**
 * Trampoline class (Python wrapper) for IEmbeddingOpenAI.
 * This interface inherits from IBaseEmbedding, which also has virtual
 * methods (including a pure virtual one: GenerateEmbeddings).
 * Therefore, we need to override:
 *  - SetAPIKey (purely virtual in IEmbeddingOpenAI)
 *  - GenerateEmbeddings (purely virtual in IBaseEmbedding)
 *  - ProcessDocument and ProcessDocuments (virtual in IBaseEmbedding)
 */
class PyIEmbeddingOpenAI : public EmbeddingOpenAI::IEmbeddingOpenAI
{
public:
    // Use base class constructor(s)
    using EmbeddingOpenAI::IEmbeddingOpenAI::IEmbeddingOpenAI;

    // Destructor
    ~PyIEmbeddingOpenAI() override = default;

    // ----------------------------------------------------------------------
    // Pure methods from IEmbeddingOpenAI
    // ----------------------------------------------------------------------
    void SetAPIKey(const std::string &apiKey) override
    {
        PYBIND11_OVERRIDE_PURE(
            void,                              // Return type
            EmbeddingOpenAI::IEmbeddingOpenAI, // Base class
            SetAPIKey,                         // Method name
            apiKey                             // Parameter
        );
    }

    // ----------------------------------------------------------------------
    // Methods (pure or virtual) inherited from IBaseEmbedding
    // ----------------------------------------------------------------------
    std::vector<RAGLibrary::Document> GenerateEmbeddings(const std::vector<RAGLibrary::Document> &documents, const std::string &model, size_t batch_size) override
    {
        PYBIND11_OVERRIDE_PURE(
            std::vector<RAGLibrary::Document>, // Type of returns
            EmbeddingOpenAI::IEmbeddingOpenAI, // Base Class
            GenerateEmbeddings,                // Name of method
            documents, model, batch_size       // Parameters
        );
    }
};

/**
 * Creates the binding for the IEmbeddingOpenAI interface.
 * Since it inherits from IBaseEmbedding (which is also abstract) and
 * has pure virtual method(s), it cannot be instantiated
 * directly in Python without overriding these methods.
 */
void bind_IEmbeddingOpenAI(py::module &m)
{
    py::class_<EmbeddingOpenAI::IEmbeddingOpenAI,
               PyIEmbeddingOpenAI,
               std::shared_ptr<EmbeddingOpenAI::IEmbeddingOpenAI>,
               Embedding::IBaseEmbedding>(
        m,
        "IEmbeddingOpenAI",
        R"doc(
            Interface for the EmbeddingOpenAI class. It inherits from IBaseEmbedding,
            which allows it to be used anywhere an IBaseEmbedding is expected.
            The main purpose of this interface is to allow dependency injection
            and decoupling, facilitating testing and maintainability. The end user
            is not expected to instantiate this class directly, but rather to use it
            as a type to instantiate a concrete object.
        )doc")
        .def(
            py::init<>(),
            R"doc(
            Default constructor for the IEmbeddingOpenAI interface.
            As it is an interface, it must be overridden in Python
            to be able to instantiate a concrete object.
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
            py::arg("model"),
            py::arg("batch_size") = 32,
            R"doc(
            Generates embeddings for a list of strings, using
            the configured model (OpenAI).
        )doc");
}
// --------------------------------------------------------------------------
// Binding para EmbeddingOpenAI::EmbeddingOpenAI
// --------------------------------------------------------------------------

/**
 * Creates the binding for the concrete class EmbeddingOpenAI, which inherits from
 * IEmbeddingOpenAI (and therefore, from IBaseEmbedding). This implementation
 * uses the OpenAI API to generate embeddings.
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
            of the OpenAI API for generating embeddings. Example usage in Python:

                from RagPUREAI import EmbeddingOpenAI

                emb = EmbeddingOpenAI()
                emb.SetAPIKey("your_openai_key")
                embeddings = emb.GenerateEmbeddings(["example text", "more text"])

            Alternatively, it can also leverage the inherited methods
            from IBaseEmbedding, such as .ProcessDocument() and .ProcessDocuments().
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
            Sets the OpenAI API key, which is required to use
            the embeddings endpoint. Internally, this key will be
            configured in the client via openai::start(apiKey).
        )doc")
        .def(
            "GenerateEmbeddings",
            &EmbeddingOpenAI::EmbeddingOpenAI::GenerateEmbeddings,
            py::arg("text"),
            py::arg("model"),
            py::arg("batch_size") = 32,
            R"doc(
            Generates embeddings for a list of strings using the
            "text-embedding-ada-002" model from OpenAI. It may raise
            a RagException if an error occurs in the JSON response.

            Parameters:
                text (list[str]): list of input texts

            Returns:
                list[float]: vector with the concatenated embedding values.
        )doc");
}

// VectorDabase
void bind_VectorDB(pybind11::module_ &);

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

    bind_ChunkCommons(m);
    bind_ContentCleaner(m);
    bind_ChunkDefault(m);
    bind_ChunkCount(m);
    bind_ChunkQuery(m);
    bind_ChunkSimilarity(m);
    bind_EmbeddingDocument(m);
    bind_IBaseEmbedding(m);
    bind_IEmbeddingOpenAI(m);
    bind_EmbeddingOpenAI(m);

    py::module_ vectorDB = m.def_submodule("vectorDB", "Bindings for vector database");
    bind_VectorDB(vectorDB);
}
