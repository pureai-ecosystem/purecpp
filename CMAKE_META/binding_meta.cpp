

// #include <pybind11>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <pybind11/complex.h>
#include <pybind11/chrono.h>
#include <pybind11/complex.h>
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
// #include <torch/torch.h>
#include <re2/re2.h>
#include <semaphore>
#include <format>
#include <iostream> 
#include <filesystem>
#include <Python.h>

// Cabeçalhos do seu projeto
#include "RagException.h"
#include "ThreadSafeQueue.h"
#include "StringUtils.h"
#include "FileUtilsLocal.h"
#include "CommonStructs.h"

#include "../components/MetadataExtractor/Document.h"
#include "IMetadataExtractor.h"
#include "MetadataExtractor.h"
#include "MetadataRegexExtractor/IMetadataRegexExtractor.h"
#include "MetadataHFExtractor/IMetadataHFExtractor.h"
#include "MetadataRegexExtractor/MetadataRegexExtractor.h"
#include "MetadataHFExtractor/MetadataHFExtractor.h"


namespace py = pybind11;
using namespace RAGLibrary;
//using namespace DataLoader;

typedef std::vector<std::pair<std::string, std::string>> test_vec_pair;
// using   test_vec_pair = std::vector<std::pair<std::string, std::string>>;

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
// Binding para MetadataExtractor::Document
//--------------------------------------------------------------------------
// 1) Remover map<int,string> do binding e usar vector<pair<string,string>>
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

// Classe trampolim para IMetadataExtractor
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
// Binding para MetadataExtractor::MetadataExtractor
// --------------------------------------------------------------------------
// Classe trampolim para permitir override de métodos em Python
class PyMetadataExtractor : public MetadataExtractor::MetadataExtractor
{
public:
    using MetadataExtractor::MetadataExtractor::MetadataExtractor;

    // Implementação em Python para método virtual puro
    ::MetadataExtractor::Document ProcessDocument(::MetadataExtractor::Document doc) override
    {
        PYBIND11_OVERRIDE_PURE(
            ::MetadataExtractor::Document,          // Return type
            ::MetadataExtractor::MetadataExtractor, // Parent class
            ProcessDocument,                        // Nome da função
            doc                                     // Parâmetros
        );
    }

    // Implementação em Python para método virtual
    std::vector<::MetadataExtractor::Document> ProcessDocuments(std::vector<::MetadataExtractor::Document> docs, const int& maxWorkers) override
    {
        PYBIND11_OVERRIDE(
            std::vector<::MetadataExtractor::Document>, // Return type
            ::MetadataExtractor::MetadataExtractor,     // Parent class
            ProcessDocuments,                           // Nome da função
            docs,                                       // Parâmetro 1
            maxWorkers                                  // Parâmetro 2
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

// Classe trampolim para IMetadataRegexExtractor
class PyIMetadataRegexExtractor : public MetadataRegexExtractor::IMetadataRegexExtractor
{
public:
    // Métodos virtuais puros de IMetadataRegexExtractor
    void AddPattern(const std::string& name, const std::string& pattern) override
    {
        PYBIND11_OVERRIDE_PURE(
            void,
            MetadataRegexExtractor::IMetadataRegexExtractor,
            AddPattern,
            name, pattern);
    }

    // Métodos herdados de IMetadataExtractor
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
// Binding para MetadataHFExtractor::IMetadataHFExtractor
// --------------------------------------------------------------------------

class PyIMetadataHFExtractor : public MetadataHFExtractor::IMetadataHFExtractor
{
public:
    // Usa os construtores da classe base
    using MetadataHFExtractor::IMetadataHFExtractor::IMetadataHFExtractor;

    // Implementa a chamada a InitializeNERModel em Python
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

    // O método ProcessDocument vem da classe base ::MetadataExtractor::MetadataExtractor
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
 * Cria o binding da interface IMetadataHFExtractor. Como é uma classe
 * puramente virtual, precisamos do trampolim `PyIMetadataHFExtractor`
 * para permitir que os métodos sejam sobrescritos em Python.
 */

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
// Binding para MetadataHFExtractor::MetadataHFExtractor
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
// Módulo principal
//--------------------------------------------------------------------------
PYBIND11_MODULE(purecpp_meta, m)
{   m.doc() = "Bindings unificados do purecpp_meta";

    bind_Document(m);
    bind_IMetadataExtractor(m);
    bind_MetadataExtractor(m);
    bind_IMetadataHFExtractor(m);
    bind_IMetadataRegexExtractor(m);
    bind_MetadataRegexExtractor(m);
    bind_MetadataHFExtractor(m);
}
