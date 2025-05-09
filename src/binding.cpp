
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
// Function of binding for RagException
//--------------------------------------------------------------------------
void bind_RagException(py::module& m)
{
    py::register_exception<RagException>(m, "RagException");
}
 
// --------------------------------------------------------------------------
// Binding for FileUtils
// --------------------------------------------------------------------------
void bind_FileUtilsLocal(py::module& m)
{
    m.def("FileReader", &RAGLibrary::FileReader,
        py::arg("filePath"),
        R"doc(
              Lê o conteúdo de um arquivo especificado por 'filePath'.
              Retorna uma std::string com todo o conteúdo do arquivo.
              Em caso de erro, levanta uma RagException.
          )doc");
}
// --------------------------------------------------------------------------
// Binding for StringUtils
// --------------------------------------------------------------------------
void bind_StringUtils(py::module& m)
{
    m.def("escapeRegex", &StringUtils::escapeRegex, py::arg("str"),
        "Escapa caracteres especiais em uma string para uso em expressões regulares.");
    m.def("joinStr", [](const std::string& str, const std::vector<std::string>& inputs)
        {
            std::string output;
            StringUtils::joinStr(str, inputs, output);
            return output; }, py::arg("str"), py::arg("inputs"), "Junta um vetor de strings com um separador fornecido em 'str'.");
 
    m.def("ellipsis", &StringUtils::ellipsis, py::arg("text"), py::arg("maxLength") = 100,
        "Trunca a string e adiciona reticências se exceder 'maxLength'.");
 
    m.def("any2str", &StringUtils::any2str, py::arg("var"),
        "Converte um std::any em uma string representativa.");
 
    m.def("str_details", &StringUtils::str_details, py::arg("text"),
        "Retorna detalhes da string fornecida.");
    m.def("removeAccents", &StringUtils::removeAccents, py::arg("input"),
        "Remove acentos da string fornecida.");
}
//--------------------------------------------------------------------------
// Template for ThreadSafeQueue
//--------------------------------------------------------------------------
template <typename Type>
void bindThreadSafeQueue(py::module& m, const std::string& name)
{
    py::class_<ThreadSafeQueue<Type>>(m, name.c_str())
        .def(py::init<>())
        .def(py::init<const ThreadSafeQueue<Type> &>())
        .def(py::init<const std::vector<Type> &>(), py::arg("vect"))
        .def("push", &ThreadSafeQueue<Type>::push, py::arg("value"))
        .def("pop", &ThreadSafeQueue<Type>::pop)
        .def("size", &ThreadSafeQueue<Type>::size);
}
// --------------------------------------------------------------------------
// Function of binding for the structs comuns of namespace RAGLibrary
// --------------------------------------------------------------------------
 
 
void bind_CommonStructs(py::module& m)
{
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
    // ThreadSafeQueue<DataExtractRequestStruct> e ThreadSafeQueue<std::string>
    // ----------------------------------------------------------------------
    bindThreadSafeQueue<RAGLibrary::DataExtractRequestStruct>(m, "ThreadSafeQueueDataRequest");
    bindThreadSafeQueue<std::string>(m, "ThreadSafeQueueString");
 
    // ----------------------------------------------------------------------
    // ThreadStruct
    // ----------------------------------------------------------------------
    py::class_<RAGLibrary::ThreadStruct>(m, "ThreadStruct",
        R"doc(
            Estrutura que representa uma 'thread' de trabalho, contendo:
              - threadRunner: um std::future<void> gerenciado por shared_ptr.
              - threadQueue: uma fila (ThreadSafeQueueDataRequest) associada.
              - threadRemainingWork: contador para trabalho restante.
        )doc")
        .def(py::init<>(),
            R"doc(
                 Construtor padrão (inicializa ponteiros e valores default).
             )doc")
        .def(py::init<
            std::shared_ptr<std::future<void>>,
            RAGLibrary::ThreadSafeQueueDataRequest,
            unsigned int>(),
            py::arg("threadRunner"),
            py::arg("threadQueue"),
            py::arg("threadRemainingWork"),
            R"doc(
                 Construtor que recebe o futuro da thread (threadRunner),
                 a fila de dados (threadQueue) e a quantidade de trabalho
                 restante (threadRemainingWork).
             )doc")
        .def_readwrite("threadRunner", &RAGLibrary::ThreadStruct::threadRunner,
            "std::shared_ptr<std::future<void>> para sincronizar o fim da thread.")
        .def_readwrite("threadQueue", &RAGLibrary::ThreadStruct::threadQueue,
            "Fila associada a essa thread.")
        .def_readwrite("threadRemainingWork", &RAGLibrary::ThreadStruct::threadRemainingWork,
            "Quantidade de trabalho restante a ser processado.");
 
    // ----------------------------------------------------------------------
    // KeywordData
    // ----------------------------------------------------------------------
    py::class_<RAGLibrary::KeywordData>(m, "KeywordData",
        R"doc(
            Estrutura que guarda informações de ocorrências de uma palavra-chave:
              - occurrences: total de ocorrências
              - position: vetor de pares (linha, offset)
        )doc")
        .def(py::init<>(),
            R"doc(
                 Construtor padrão, inicializa occurrences = 0 e position vazio.
             )doc")
        .def_readwrite("occurrences", &RAGLibrary::KeywordData::occurrences,
            "Número total de ocorrências encontradas.")
        .def_readwrite("position", &RAGLibrary::KeywordData::position,
            R"doc(
                 Posições de cada ocorrência como pares (linha, offset).
             )doc");
 
    // ----------------------------------------------------------------------
    // UpperKeywordData
    // ----------------------------------------------------------------------
    py::class_<RAGLibrary::UpperKeywordData>(m, "UpperKeywordData",
        R"doc(
            Estrutura que agrega as ocorrências de uma keyword em vários arquivos,
            incluindo:
              - totalOccurences: total geral
              - keywordDataPerFile: map<string, KeywordData> (por arquivo)
        )doc")
        .def(py::init<>(),
            R"doc(
                 Construtor padrão, inicializa totalOccurences = 0.
             )doc")
        .def_readwrite("totalOccurences", &RAGLibrary::UpperKeywordData::totalOccurences,
            "Total de ocorrências da keyword em todos os arquivos.")
        .def_readwrite("keywordDataPerFile", &RAGLibrary::UpperKeywordData::keywordDataPerFile,
            R"doc(
                 Mapeia o nome do arquivo (string) -> dados da keyword (KeywordData).
             )doc")
        .def("__str__", [](const RAGLibrary::UpperKeywordData& data)
            {
                std::ostringstream oss;
                oss << data;
                return oss.str(); });
 
    // ----------------------------------------------------------------------
    // Document ( 'RAGDocument' in Python)
    // ----------------------------------------------------------------------
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
// --------------------------------------------------------------------------
// Binding for MetadataExtractor::Document
//--------------------------------------------------------------------------
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
                Construtor de Document.

                Parâmetros:
                    pageContent (list[str]): lista de strings que representam o conteúdo do documento.
                    metadata (list[tuple[str, str]]): lista de pares (chave, valor) para o metadado.
            )doc")
        .def_readwrite("pageContent", &::MetadataExtractor::Document::pageContent, R"doc(
                Campo que armazena o conteúdo do documento em forma de vetor de strings.
            )doc")
        .def_readwrite("metadata", &::MetadataExtractor::Document::metadata, R"doc(
                Metadados do documento, armazenados em pares (chave, valor).
            )doc")
        .def("StringRepr", &::MetadataExtractor::Document::StringRepr,
            R"doc(
                Retorna uma string de representação que lista cada metadado contido no documento.
            )doc");

    bindThreadSafeQueue2<::MetadataExtractor::Document>(m, "ThreadSafeQueueDocument");
}

// Class trampolim for IMetadataExtractor
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
            ::MetadataExtractor::Document,          
            ::MetadataExtractor::MetadataExtractor, 
            ProcessDocument,                        
            doc                                     
        );
    }

    std::vector<::MetadataExtractor::Document> ProcessDocuments(std::vector<::MetadataExtractor::Document> docs, const int& maxWorkers) override
    {
        PYBIND11_OVERRIDE(
            std::vector<::MetadataExtractor::Document>, 
            ::MetadataExtractor::MetadataExtractor,    
            ProcessDocuments,                           
            docs,                                       
            maxWorkers                                  
        );
    }
};

void bind_MetadataExtractor(py::module& m)
{
    py::class_<MetadataExtractor::MetadataExtractor, PyMetadataExtractor, std::shared_ptr<MetadataExtractor::MetadataExtractor>, MetadataExtractor::IMetadataExtractor>(
        m,
        "MetadataExtractor",
        R"doc(
            Classe base para extração de metadados, herdada de IMetadataExtractor.
            Possui um método virtual puro para processar um único documento, e
            um método para processar múltiplos documentos em paralelo.
        )doc")
        .def(
            py::init<>(),
            R"doc(
            Construtor padrão para a classe MetadataExtractor.
        )doc")
        .def(
            "ProcessDocument",
            &MetadataExtractor::MetadataExtractor::ProcessDocument,
            py::arg("doc"),
            R"doc(
            Processa metadados de um único documento.
            Este método é virtual puro e deve ser sobrescrito
            em classes derivadas concretas.
        )doc")
        .def(
            "ProcessDocuments",
            &MetadataExtractor::MetadataExtractor::ProcessDocuments,
            py::arg("docs"),
            py::arg("maxWorkers") = 4,
            R"doc(
            Processa metadados de vários documentos, com suporte a paralelismo.
            Por padrão, usa até 4 threads (maxWorkers=4).
        )doc");
}

// Class trampolim for IMetadataRegexExtractor
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
            Interface que herda de MetadataExtractor::MetadataExtractor e adiciona
            métodos para inicializar um modelo NER e extrair metadados.
        )doc")
        .def(
            py::init<>(),
            R"doc(
            Construtor padrão da interface IMetadataHFExtractor. 
            Note que não pode ser instanciada sem uma subclasse concreta.
        )doc")
        .def(
            "InitializeNERModel",
            &MetadataHFExtractor::IMetadataHFExtractor::InitializeNERModel,
            R"doc(
            Método puro que deve ser sobrescrito para carregar e inicializar 
            o modelo de Named Entity Recognition (NER).
        )doc")
        .def(
            "ExtractMetadata",
            &MetadataHFExtractor::IMetadataHFExtractor::ExtractMetadata,
            py::arg("text"),
            R"doc(
            Método puro para extrair metadados (entidades nomeadas) de um ou mais
            textos. Retorna um vetor de pares (token, entidade).
        )doc")
        .def(
            "ProcessDocument",
            &MetadataHFExtractor::IMetadataHFExtractor::ProcessDocument,
            py::arg("doc"),
            R"doc(
            Método puro que processa metadados em um documento do tipo 
            MetadataExtractor::Document. Pode incluir lógica de NER.
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
            Classe concreta que implementa a extração de metadados utilizando modelos
            de NER via ONNXRuntime e bibliotecas de tokenização (HuggingFace tokenizers).
        )doc")
        .def(
            py::init<>(),
            R"doc(
            Construtor padrão que configura o ambiente ONNXRuntime, sessões e 
            tokenizers necessários para a extração de metadados.
        )doc")
        .def(
            "InitializeNERModel",
            &MetadataHFExtractor::MetadataHFExtractor::InitializeNERModel,
            R"doc(
            Inicializa o modelo de NER carregando o arquivo ONNX e preparando
            o ambiente de inferência (sessão ONNXRuntime, CPU provider, etc.).
        )doc")
        .def(
            "ExtractMetadata",
            &MetadataHFExtractor::MetadataHFExtractor::ExtractMetadata,
            py::arg("text"),
            R"doc(
            Executa a extração de metadados (entidades nomeadas) em um ou mais
            textos fornecidos. Retorna um vetor de pares (entidade, rótulo).
            
            Parâmetros:
                text (list[str]): Lista de strings que serão processadas.

            Retorna:
                list[tuple[str, str]]: Cada elemento é um par (token, entidade).
        )doc")
        .def(
            "ProcessDocument",
            &MetadataHFExtractor::MetadataHFExtractor::ProcessDocument,
            py::arg("doc"),
            R"doc(
            Processa metadados em um objeto `MetadataExtractor.Document`, incluindo
            detecção de entidades nomeadas. Retorna o mesmo objeto `Document`, mas
            com metadados atualizados.

            Parâmetros:
                doc (MetadataExtractor.Document): Documento a ser processado.

            Retorna:
                MetadataExtractor.Document: Documento com entidades nomeadas
                adicionadas a seu conjunto de metadados.
        )doc");
}

//--------------------------------------------------------------------------
// Classe trampolim para IBaseDataLoader
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
 
    bool KeywordExists(const std::string& fileName, const std::string& keyword) override
    {
        PYBIND11_OVERRIDE_PURE(
            bool,
            IBaseDataLoader,
            KeywordExists,
            fileName, keyword);
    }
 
    UpperKeywordData GetKeywordOccurences(const std::string& keyword) override
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
 
 
//--------------------------------------------------------------------------
// Class trampolim for BaseDataLoader
//--------------------------------------------------------------------------
class PyBaseDataLoader : public BaseDataLoader
{
public:
    PyBaseDataLoader(unsigned int threadsNum)
        : BaseDataLoader(threadsNum) {
    }
 
    // void InsertDataToExtract(const std::vector<DataExtractRequestStruct>& dataPaths) override
    // {
    //     PYBIND11_OVERRIDE_PURE(
    //         void,
    //         BaseDataLoader,
    //         InsertDataToExtract,
    //         dataPaths);
    // }
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
// Bind for PDFLoader
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
// Binding for Embedding::Document
// --------------------------------------------------------------------------

void bind_EmbeddingDocument(py::module &m)
{
    // ----------------------------------------------------------------------
    // Class Embedding::Document --> Python: EmbeddingDocument
    // ----------------------------------------------------------------------
    py::class_<Embedding::Document>(
        m,
        "EmbeddingDocument",
        R"doc(
            Estrutura de documento para embeddings, contendo:

              - pageContent: Vetor de strings representando o conteúdo.
              - metadata: Vetor de pares (chave, valor) relacionados ao conteúdo.
              - embeddings: Vetor de floats representando os vetores de embedding.
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
            Construtor principal que recebe:

              - pageContent (list[str]): Lista de conteúdo.
              - metadata (list[tuple[str, str]]): Lista de pares chave-valor como metadados.
              - embeddings (list[float]): Embeddings (vetor de floats).
        )doc")
        .def(
            py::init<const ::MetadataExtractor::Document &>(),
            py::arg("document"),
            R"doc(
            Construtor que converte um 'MetadataExtractor::Document'
            em um 'EmbeddingDocument'.
        )doc")
        .def(
            "StringRepr",
            &Embedding::Document::StringRepr,
            R"doc(
            Retorna uma representação textual do documento, 
            incluindo metadata e embeddings se existirem.
        )doc")
        .def_readwrite("pageContent", &Embedding::Document::pageContent)
        .def_readwrite("metadata", &Embedding::Document::metadata)
        .def_readwrite("embeddings", &Embedding::Document::embeddings);

    // ----------------------------------------------------------------------
    // Opcional: Binding for ThreadSafeQueue<Embedding::Document>
    // ----------------------------------------------------------------------
    py::class_<Embedding::ThreadSafeQueueDocument>(
        m,
        "ThreadSafeQueueEmbeddingDocument",
        R"doc(
            Fila thread-safe de EmbeddingDocument, permitindo acesso concorrente
            em cenários com paralelismo.
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
    //~PyIBaseEmbedding() override = default;
    ~PyIBaseEmbedding() = default;

    std::vector<RAGLibrary::Document> GenerateEmbeddings(const std::vector<RAGLibrary::Document> &documents, const std::string &model) override
    {
        PYBIND11_OVERRIDE_PURE(
            std::vector<RAGLibrary::Document>, // Type class
            Embedding::IBaseEmbedding,         // Base class
            GenerateEmbeddings,                // Name of method
            documents, model                   // Parameters
        );
    }
};

void bind_IBaseEmbedding(py::module &m)
{
    py::class_<Embedding::IBaseEmbedding, PyIBaseEmbedding, Embedding::IBaseEmbeddingPtr>(
        m,
        "IBaseEmbedding",
        R"doc(
            Interface base para geração de embeddings em textos ou documentos.
            Possui métodos para gerar embeddings, processar um documento,
            e processar vários documentos em paralelo.
        )doc")
        .def(
            py::init<>(),
            R"doc(
            Construtor padrão da interface IBaseEmbedding. 
            Não pode ser instanciada sem sobrescrever métodos em Python.
        )doc")
        .def(
            "GenerateEmbeddings",
            &Embedding::IBaseEmbedding::GenerateEmbeddings,
            py::arg("text"),
            py::arg("model"),
            R"doc(
            Gera embeddings para um vetor de strings.
            
            Parâmetros:
                text (list[str]): lista de textos a serem convertidos em embeddings
            
            Retorna:
                list[float]: vetor de floats representando as embeddings concatenadas.
        )doc");
}
// --------------------------------------------------------------------------
// Binding for Embedding::BaseEmbedding
// --------------------------------------------------------------------------
class PyBaseEmbedding : public Embedding::BaseEmbedding
{
public:
    using Embedding::BaseEmbedding::BaseEmbedding;

    std::vector<RAGLibrary::Document> GenerateEmbeddings(const std::vector<RAGLibrary::Document> &documents, const std::string &model) override
    {
        PYBIND11_OVERRIDE_PURE(
            std::vector<RAGLibrary::Document>,
            Embedding::BaseEmbedding,
            GenerateEmbeddings,
            documents, model);
    }
};

void bind_BaseEmbedding(py::module &m)
{
    py::class_<Embedding::BaseEmbedding, PyBaseEmbedding, std::shared_ptr<Embedding::BaseEmbedding>, Embedding::IBaseEmbedding>(
        m,
        "BaseEmbedding",
        R"doc(
            Classe abstrata que implementa parte da lógica de embeddings, herdando
            de IBaseEmbedding. Ainda possui um método puramente virtual 
            (GenerateEmbeddings), tornando-a não instanciável diretamente.
        )doc")
        .def(
            py::init<>(),
            R"doc(
            Construtor padrão da classe BaseEmbedding.
        )doc")
        .def(
            "GenerateEmbeddings",
            &Embedding::BaseEmbedding::GenerateEmbeddings,
            py::arg("text"),
            py::arg("model"),
            R"doc(
            Método puramente virtual que gera embeddings para um conjunto 
            de strings. Deve ser sobrescrito por classes derivadas concretas.
        )doc");
}
// --------------------------------------------------------------------------
// Binding for EmbeddingOpenAI::IEmbeddingOpenAI
// --------------------------------------------------------------------------
class PyIEmbeddingOpenAI : public EmbeddingOpenAI::IEmbeddingOpenAI
{
public:
    using EmbeddingOpenAI::IEmbeddingOpenAI::IEmbeddingOpenAI;

    // Destrutor
    ~PyIEmbeddingOpenAI() override = default;

    // ----------------------------------------------------------------------
    // IEmbeddingOpenAI
    // ----------------------------------------------------------------------
    void SetAPIKey(const std::string &apiKey) override
    {
        PYBIND11_OVERRIDE_PURE(
            void,                              // Type of returns
            EmbeddingOpenAI::IEmbeddingOpenAI, // Base Class
            SetAPIKey,                         // Name of method
            apiKey                             // Parameters
        );
    }

    // ----------------------------------------------------------------------
    // (virtuals) BaseEmbedding
    // ----------------------------------------------------------------------
    std::vector<RAGLibrary::Document> GenerateEmbeddings(const std::vector<RAGLibrary::Document> &documents, const std::string &model) override
    {
        PYBIND11_OVERRIDE_PURE(
            std::vector<RAGLibrary::Document>, // Type of returns
            EmbeddingOpenAI::IEmbeddingOpenAI, // Base Class
            GenerateEmbeddings,                // Name of method
            documents, model                   // Parameters
        );
    }
};

void bind_IEmbeddingOpenAI(py::module &m)
{
    py::class_<EmbeddingOpenAI::IEmbeddingOpenAI,
               PyIEmbeddingOpenAI,
               EmbeddingOpenAI::IEmbeddingOpenAIPtr,
               Embedding::BaseEmbedding>(
        m,
        "IEmbeddingOpenAI",
        R"doc(
            Interface que herda de Embedding::BaseEmbedding e adiciona o método
            puramente virtual 'SetAPIKey' para configuração da chave de API 
            necessária para gerar embeddings utilizando OpenAI.
            
            Métodos principais:
             - SetAPIKey(apiKey: str) -> None
             - GenerateEmbeddings(documents: list[Document], model: str) -> list[Document]
        )doc")
        .def(
            py::init<>(),
            R"doc(
            Construtor padrão para a interface IEmbeddingOpenAI.
            Como é uma interface, deve ser sobrescrita em Python 
            para poder instanciar um objeto concreto.
        )doc")
        .def(
            "SetAPIKey",
            &EmbeddingOpenAI::IEmbeddingOpenAI::SetAPIKey,
            py::arg("apiKey"),
            R"doc(
            Define a chave de API a ser utilizada para gerar embeddings
            (no caso, a chave de API da OpenAI).
        )doc")
        .def(
            "GenerateEmbeddings",
            &EmbeddingOpenAI::IEmbeddingOpenAI::GenerateEmbeddings,
            py::arg("text"),
            py::arg("model"),
            R"doc(
            Gera embeddings para uma lista de strings, usando 
            o modelo configurado (OpenAI).
        )doc");
}
// --------------------------------------------------------------------------
// Binding for EmbeddingOpenAI::EmbeddingOpenAI
// --------------------------------------------------------------------------
void bind_EmbeddingOpenAI(py::module &m)
{
    py::class_<EmbeddingOpenAI::EmbeddingOpenAI,
               std::shared_ptr<EmbeddingOpenAI::EmbeddingOpenAI>,
               EmbeddingOpenAI::IEmbeddingOpenAI>
        cls(
            m,
            "EmbeddingOpenAI",
            R"doc(
            Classe concreta que implementa IEmbeddingOpenAI, permitindo o uso 
            da API OpenAI para geração de embeddings. Exemplo de uso em Python:

                from RagPUREAI import EmbeddingOpenAI

                emb = EmbeddingOpenAI()
                emb.SetAPIKey("sua_chave_openai")
                embeddings = emb.GenerateEmbeddings(["texto de exemplo", "mais texto"])

            ou, alternativamente, também pode aproveitar os métodos herdados
            de BaseEmbedding, como .ProcessDocument() e .ProcessDocuments().
        )doc");

    cls.def(
           py::init<>(),
           R"doc(
            Construtor padrão da classe EmbeddingOpenAI.
        )doc")
        .def(
            "SetAPIKey",
            &EmbeddingOpenAI::EmbeddingOpenAI::SetAPIKey,
            py::arg("apiKey"),
            R"doc(
            Define a chave de API da OpenAI, necessária para utilizar
            o endpoint de embeddings. Internamente, esta chave será 
            configurada no cliente openai::start(apiKey).
        )doc")
        .def(
            "GenerateEmbeddings",
            &EmbeddingOpenAI::EmbeddingOpenAI::GenerateEmbeddings,
            py::arg("text"),
            py::arg("model"),
            R"doc(
            Gera embeddings para uma lista de strings, utilizando o 
            modelo "text-embedding-ada-002" da OpenAI. Pode levantar 
            RagException caso ocorra algum erro na resposta JSON.

            Parâmetros:
                text (list[str]): lista de textos de entrada

            Retorna:
                list[float]: vetor com os valores do embedding concatenados.
        )doc");
}

//--------------------------------------------------------------------------
// Function binding for the ChunkCommon
//--------------------------------------------------------------------------
void bind_ChunkCommons(py::module& m)
{
    //--------------------------------------------------------------------------
    // Binding for the enum EmbeddingModel
    //--------------------------------------------------------------------------
 
    py::enum_<Chunk::EmbeddingModel>(m, "EmbeddingModel", R"doc(
        Enumeração dos modelos de embedding suportados:
        
        - HuggingFace: Utiliza modelos da HuggingFace.
        - OpenAI: Utiliza modelos da OpenAI.
    )doc")
        .value("HuggingFace", Chunk::EmbeddingModel::HuggingFace)
        .value("OpenAI", Chunk::EmbeddingModel::OpenAI)
        .export_values();
 
    //--------------------------------------------------------------------------
    // Binding for the function MeanPooling
    //--------------------------------------------------------------------------
    m.def("MeanPooling", &Chunk::MeanPooling,
        py::arg("token_embeddings"), py::arg("attention_mask"), py::arg("embedding_size"),
        R"doc(
              Calcula a média das embeddings com base no attention mask.
              
              Parâmetros:
                  token_embeddings (list[float]): Lista de embeddings de tokens.
                  attention_mask (list[int]): Lista de máscaras de atenção (1 ou 0).
                  embedding_size (int): Tamanho das embeddings.
              
              Retorna:
                  list[float]: Lista de embeddings médias.
          )doc");
 
    //--------------------------------------------------------------------------
    // Binding for the function NormalizeEmbeddings
    //--------------------------------------------------------------------------
    m.def("NormalizeEmbeddings", &Chunk::NormalizeEmbeddings,
        py::arg("embeddings"),
        R"doc(
              Normaliza as embeddings para que tenham norma 1.
              
              Parâmetros:
                  embeddings (list[float]): Lista de embeddings a serem normalizadas.
          )doc");
 
    //--------------------------------------------------------------------------
    // Binding for the function EmbeddingModelBatch
    //--------------------------------------------------------------------------
    m.def("EmbeddingModelBatch", &Chunk::EmbeddingModelBatch,
        py::arg("chunks"), py::arg("model"), py::arg("batch_size") = 32,
        R"doc(
              Gera embeddings para um lote de chunks usando um modelo especificado.
              
              Parâmetros:
                  chunks (list[str]): Lista de strings a serem embedadas.
                  model (str): Nome do modelo a ser usado.
                  batch_size (int, opcional): Tamanho do lote (padrão=32).
              
              Retorna:
                  list[list[float]]: Lista de listas de embeddings.
          )doc");
 
    //--------------------------------------------------------------------------
    // Binding for the function EmbeddingHuggingFaceTransformers
    //--------------------------------------------------------------------------
    m.def("EmbeddingHuggingFaceTransformers", &Chunk::EmbeddingHuggingFaceTransformers,
        py::arg("chunks"),
        R"doc(
              Gera embeddings usando o modelo HuggingFace 'sentence-transformers/all-MiniLM-L6-v2'.
              
              Parâmetros:
                  chunks (list[str]): Lista de strings a serem embedadas.
              
              Retorna:
                  list[list[float]]: Lista de listas de embeddings.
          )doc");
 
    //--------------------------------------------------------------------------
    // Binding for the function EmbeddingOpeanAI
    //--------------------------------------------------------------------------
    m.def("EmbeddingOpeanAI", &Chunk::EmbeddingOpeanAI,
        py::arg("chunks"), py::arg("openai_api_key"),
        R"doc(
              Gera embeddings usando a API da OpenAI.
              
              Parâmetros:
                  chunks (list[str]): Lista de strings a serem embedadas.
                  openai_api_key (str): Chave de API da OpenAI.
              
              Retorna:
                  list[list[float]]: Lista de listas de embeddings.
          )doc");
 
    //--------------------------------------------------------------------------
    // Binding for the function toTensor
    //--------------------------------------------------------------------------
    m.def("toTensor", &Chunk::toTensor,
        py::arg("vect"),
        R"doc(
              Converte uma lista de listas de floats em um tensor do PyTorch.
              
              Parâmetros:
                  vect (list[list[float]]): Lista de listas de floats.
              
              Retorna:
                  torch.Tensor: Tensor do PyTorch.
          )doc");
 
    //--------------------------------------------------------------------------
    // Binding for the function SplitText
    //--------------------------------------------------------------------------
    m.def("SplitText", &Chunk::SplitText,
        py::arg("inputs"), py::arg("overlap"), py::arg("chunk_size"),
        R"doc(
              Divide o texto em chunks com sobreposição.
              
              Parâmetros:
                  inputs (list[str]): Lista de strings de entrada.
                  overlap (int): Número de caracteres para sobreposição.
                  chunk_size (int): Tamanho de cada chunk.
              
              Retorna:
                  list[str]: Lista de chunks resultantes.
          )doc");
 
    //--------------------------------------------------------------------------
    // Binding for the function SplitTextByCount
    //--------------------------------------------------------------------------
    m.def("SplitTextByCount", &Chunk::SplitTextByCount,
        py::arg("inputs"), py::arg("overlap"), py::arg("count_threshold"), py::arg("regex"),
        R"doc(
              Divide o texto em chunks baseado em contagem e regex.
              
              Parâmetros:
                  inputs (list[str]): Lista de strings de entrada.
                  overlap (int): Número de caracteres para sobreposição.
                  count_threshold (int): Limite de contagem para divisão.
                  regex (re2.RE2): Expressão regular para identificar pontos de divisão.
              
              Retorna:
                  list[str]: Lista de chunks resultantes.
          )doc");
}
 
//--------------------------------------------------------------------------
// Binding for ChunkDefault
//--------------------------------------------------------------------------
void bind_ChunkDefault(py::module& m)
{
    py::class_<Chunk::ChunkDefault>(m, "ChunkDefault")
        .def(py::init<const int, const int>(),
             py::arg("chunk_size") = 100, py::arg("overlap") = 20)
 
        .def("ProcessSingleDocument",
             &Chunk::ChunkDefault::ProcessSingleDocument,
             py::arg("item"))
 
        .def("ProcessDocuments",
             &Chunk::ChunkDefault::ProcessDocuments,
             py::arg("items"), py::arg("max_workers") = 4);
}
 
//--------------------------------------------------------------------------
// Binding para ChunkCount
//--------------------------------------------------------------------------
void bind_ChunkCount(py::module& m)
{
    py::class_<Chunk::ChunkCount>(m, "ChunkCount")
        .def(py::init<const std::string&, const int, const int>(),
            py::arg("count_unit"), py::arg("overlap") = 600, py::arg("count_threshold") = 1)
        .def(py::init<>()) // Construtor padrão também existe
 
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
        Classe para processar RAGDocument e gerar chunks e embeddings,
        permitindo avaliar a similaridade de documentos. Possui opções para
        definir tamanho de chunk, overlap, modelo de embedding (HuggingFace ou OpenAI)
        e chave de API para OpenAI se necessário.
    )doc")
        .def(
            py::init<int, int, Chunk::EmbeddingModel, const std::string&>(),
            py::arg("chunk_size") = 100,
            py::arg("overlap") = 20,
            py::arg("embedding_model") = Chunk::EmbeddingModel::HuggingFace,
            py::arg("openai_api_key") = "",
            R"doc(
                Construtor que inicializa a classe ChunkSimilarity.
 
                Parâmetros:
                    chunk_size (int): Tamanho padrão de cada chunk de texto (default=100).
                    overlap (int): Sobreposição entre chunks sucessivos (default=20).
                    embedding_model (EmbeddingModel): Modelo de embedding (HuggingFace ou OpenAI).
                    openai_api_key (str): Chave de API da OpenAI (apenas se embedding_model=OpenAI).
            )doc")
        .def(
            "ProcessSingleDocument",
            &Chunk::ChunkSimilarity::ProcessSingleDocument,
            py::arg("item"),
            R"doc(
                Dado um único RAGDocument, divide seu conteúdo em chunks
                e gera embeddings de acordo com o modelo escolhido, retornando
                um vetor de RAGLibrary::Document.
 
                Parâmetros:
                    item (RAGDocument): Estrutura que contém
                        o identificador e o conteúdo textual.
 
                Retorna:
                    list[RAGDocument]: Vetor de documentos resultantes,
                    cada um com seu conteúdo chunkado e possivelmente
                    com embeddings associados em seu metadata.
            )doc")
        .def(
            "ProcessDocuments",
            &Chunk::ChunkSimilarity::ProcessDocuments,
            py::arg("items"),
            py::arg("max_workers") = 4,
            R"doc(
                Similar a ProcessSingleDocument, porém processa múltiplos
                RAGDocument em paralelo (até max_workers).
 
                Parâmetros:
                    items (list[RAGDocument]): Lista de
                        estruturas contendo o conteúdo a ser chunkado.
                    max_workers (int): Número máximo de threads a utilizar
                        no processamento paralelo (default=4).
 
                Retorna:
                    list[RAGDocument]: Lista concatenada de documentos
                    resultantes do chunk para cada item.
            )doc");
}
//--------------------------------------------------------------------------
// Function binding for ChunkQuery
//--------------------------------------------------------------------------
// Precondition: Document binding must be defined before this function
// void bind_RAGLibrary_Document(py::module &m);
void bind_ChunkQuery(py::module& m) {
    py::class_<Chunk::ChunkQuery, std::shared_ptr<Chunk::ChunkQuery>>(m, "ChunkQuery",
        R"doc(
            Class for processing queries over text chunks, generating embeddings,
            and evaluating similarity against a provided query.
        )doc")
        
        .def(py::init<
                std::string&,                       
                RAGLibrary::Document&,             // 
                std::vector<RAGLibrary::Document>&,// 
                const Chunk::EmbeddingModel,       // 
                const std::string&                 // 
            >(),
            py::arg("query") = "",
            py::arg("query_doc") = RAGLibrary::Document(),
            py::arg("chunks_list") = std::vector<RAGLibrary::Document>(),
            py::arg("embedding_model") = Chunk::EmbeddingModel::OpenAI,
            py::arg("model") = "text-embedding-ada-002",
            R"doc(
                Constructor for ChunkQuery.

                Parameters:
                    query (str, optional): Text to be processed (default="").
                    query_doc (RAGLibrary.Document, optional): Document with content/embedding (default empty).
                    chunks_list (list[RAGLibrary.Document], optional): Documents to index into chunks (default empty list).
                    embedding_model (EmbeddingModel, optional): Embedding model to use (OpenAI or HuggingFace).
                    model (str, optional): Name of the embedding model (default="text-embedding-ada-002").
            )doc")
        // Método Query
         // Sobrecarga Query(string)
         .def("Query",
            py::overload_cast<std::string>(&Chunk::ChunkQuery::Query),
            py::arg("query"),
            R"doc(
                 Generates an embedding for the provided query and returns it as a document.

                 Parameters:
                     query (str): The text query to embed.

                 Returns:
                     RAGLibrary.Document: Document containing the embedding of the query.
             )doc")
       
       .def("Query",
            py::overload_cast<RAGLibrary::Document>(&Chunk::ChunkQuery::Query),
            py::arg("query_doc"),
            R"doc(
                Uses the provided Document (with or without embedding) to set the query.
            )doc")

        
        .def("CreateVD", &Chunk::ChunkQuery::CreateVD,
             py::arg("chunks_list"),
             R"doc(
                 Creates embeddings for a list of chunks.

                 Parameters:
                     chunks_list (list[RAGLibrary.Document]): Documents for which to generate embeddings.

                 Returns:
                     list[list[float]]: Embedding vectors for each chunk.
             )doc")
        
        .def("Retrieve", &Chunk::ChunkQuery::Retrieve,
             py::arg("threshold"),
             R"doc(
                 Retrieves chunks whose similarity to the query embedding exceeds the threshold.

                 Parameters:
                     threshold (float): Similarity cutoff in [-1.0, 1.0].

                 Returns:
                     list[tuple(str, float, int)]: Triplets of
                         - chunk content (str),
                         - similarity score (float),
                         - original chunk index (int).
             )doc")
        
        .def("getRetrieveList", &Chunk::ChunkQuery::getRetrieveList,
             R"doc(
                 Returns the list of retrieved chunks with their similarity scores and original indices.

                 Returns:
                     list[tuple(str, float, int)]: Retrieved chunk content, similarity score, and original index triplets.
             )doc")
        
        .def("StrQ", &Chunk::ChunkQuery::StrQ,
             py::arg("index"),
             R"doc(
                 Formats the query and the specified retrieved chunk into a full prompt.

                 Parameters:
                     index (int): Index of the retrieved chunk to format.

                 Returns:
                     str: Formatted prompt containing question, context, similarity score, and original chunk index.
             )doc")
        .def("getQuery", &Chunk::ChunkQuery::getQuery,
            R"doc(
                Returns the current query Document.
            )doc")
        .def("getPar", &Chunk::ChunkQuery::getPar,
            R"doc(
                Returns a pair (EmbeddingModel, model name).
            )doc")
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
