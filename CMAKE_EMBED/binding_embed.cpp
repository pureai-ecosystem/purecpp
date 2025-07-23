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

#include "../components/Embedding/Document.h"
#include "IBaseEmbedding.h"
#include "BaseEmbedding.h"
#include "EmbeddingOpenAI/IEmbeddingOpenAI.h"
#include "EmbeddingOpenAI/EmbeddingOpenAI.h"

namespace py = pybind11;
using namespace RAGLibrary;
// using namespace DataLoader;

typedef std::vector<std::pair<std::string, std::string>> test_vec_pair;
// using   test_vec_pair = std::vector<std::pair<std::string, std::string>>;

// --------------------------------------------------------------------------
// Binding para MetadataExtractor::Document
//--------------------------------------------------------------------------
// 1) Remover map<int,string> do binding e usar vector<pair<string,string>>
// void bind_Document(py::module& m)
//{
//    py::class_<::MetadataExtractor::Document>(m, "Document")
//        .def(
//            py::init<
//            const std::vector<std::string> &,
//            const std::vector<std::pair<std::string, std::string>> &>(),
//            py::arg("pageContent"),
//            py::arg("metadata") = std::vector<std::pair<std::string, std::string>>{},
//            R"doc(
//                Construtor de Document.
//
//                Parâmetros:
//                    pageContent (list[str]): lista de strings que representam o conteúdo do documento.
//                    metadata (list[tuple[str, str]]): lista de pares (chave, valor) para o metadado.
//            )doc")
//        .def_readwrite("pageContent", &::MetadataExtractor::Document::pageContent, R"doc(
//                Campo que armazena o conteúdo do documento em forma de vetor de strings.
//            )doc")
//        .def_readwrite("metadata", &::MetadataExtractor::Document::metadata, R"doc(
//                Metadados do documento, armazenados em pares (chave, valor).
//            )doc")
//        .def("StringRepr", &::MetadataExtractor::Document::StringRepr,
//            R"doc(
//                Retorna uma string de representação que lista cada metadado contido no documento.
//            )doc");
//
//    bindThreadSafeQueue<::MetadataExtractor::Document>(m, "ThreadSafeQueueDocument");
//}

// --------------------------------------------------------------------------
// Binding para Embedding::Document
// --------------------------------------------------------------------------

/**
 * Para evitar conflitos com a classe 'Document' de outros namespaces (RAGLibrary,
 * MetadataExtractor), vamos expor esta estrutura como 'EmbeddingDocument' no Python.
 */
void bind_EmbeddingDocument(py::module &m)
{
    // ----------------------------------------------------------------------
    // Classe Embedding::Document --> Python: EmbeddingDocument
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
    // Opcional: Binding para a ThreadSafeQueue<Embedding::Document>
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
// Binding para Embedding::IBaseEmbedding
// --------------------------------------------------------------------------

/**
 * Classe trampolim (wrapper em Python) para IBaseEmbedding, permitindo
 * a sobrescrita dos métodos virtuais puros em Python.
 */
class PyIBaseEmbedding : public Embedding::IBaseEmbedding
{
public:
    // Destrutor padrão
    //~PyIBaseEmbedding() override = default;
    ~PyIBaseEmbedding() = default;
    // Métodos virtuais puros de IBaseEmbedding

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

/**
 * Cria o binding para a interface IBaseEmbedding.
 * Observação: Como é uma interface com métodos puros,
 * não pode ser instanciada diretamente sem uma classe concreta,
 * mas podemos herdar em Python usando a classe trampolim.
 */
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
// Binding para Embedding::BaseEmbedding
// --------------------------------------------------------------------------

/**
 * Classe trampolim (wrapper para Python) para BaseEmbedding, pois essa classe
 * ainda possui um método puramente virtual (GenerateEmbeddings). Precisamos
 * permitir a sobrescrita desse método em Python.
 */
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

/**
 * Cria o binding para a classe BaseEmbedding, que herda de IBaseEmbedding.
 * A classe BaseEmbedding ainda possui um método puramente virtual
 * (GenerateEmbeddings), então ela não pode ser instanciada diretamente.
 */
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
// Binding para EmbeddingOpenAI::IEmbeddingOpenAI
// --------------------------------------------------------------------------

/**
 * Classe trampolim (wrapper para Python) para IEmbeddingOpenAI.
 * Esta interface herda de BaseEmbedding, que também possui métodos
 * virtuais (incluindo um puramente virtual: GenerateEmbeddings).
 * Portanto, precisamos sobrescrever:
 *  - SetAPIKey (puramente virtual em IEmbeddingOpenAI)
 *  - GenerateEmbeddings (puramente virtual em BaseEmbedding)
 *  - ProcessDocument e ProcessDocuments (virtuais em BaseEmbedding)
 */
class PyIEmbeddingOpenAI : public EmbeddingOpenAI::IEmbeddingOpenAI
{
public:
    // Usa construtor(es) da classe base
    using EmbeddingOpenAI::IEmbeddingOpenAI::IEmbeddingOpenAI;

    // Destrutor
    ~PyIEmbeddingOpenAI() override = default;

    // ----------------------------------------------------------------------
    // Métodos puros de IEmbeddingOpenAI
    // ----------------------------------------------------------------------
    void SetAPIKey(const std::string &apiKey) override
    {
        PYBIND11_OVERRIDE_PURE(
            void,                              // Tipo de retorno
            EmbeddingOpenAI::IEmbeddingOpenAI, // Classe base
            SetAPIKey,                         // Nome do método
            apiKey                             // Parâmetro
        );
    }

    // ----------------------------------------------------------------------
    // Métodos (puros ou virtuais) herdados de BaseEmbedding
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

/**
 * Cria o binding para a interface IEmbeddingOpenAI.
 * Como ela herda de BaseEmbedding (que também é abstrata) e
 * possui método(s) puramente virtual(is), não pode ser instanciada
 * diretamente em Python sem sobrescrever esses métodos.
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
// Binding para EmbeddingOpenAI::EmbeddingOpenAI
// --------------------------------------------------------------------------

/**
 * Cria o binding para a classe concreta EmbeddingOpenAI, que herda de
 * IEmbeddingOpenAI (e portanto, de BaseEmbedding). Esta implementação
 * utiliza a API da OpenAI para gerar embeddings.
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
// Módulo principal
//--------------------------------------------------------------------------
PYBIND11_MODULE(purecpp_embed, m)
{
    m.doc() = "Bindings unificados do purecpp_embed";

    // bind_Document(m);

    bind_EmbeddingDocument(m);
    bind_IBaseEmbedding(m);
    bind_BaseEmbedding(m);
    bind_IEmbeddingOpenAI(m);
    bind_EmbeddingOpenAI(m);
}