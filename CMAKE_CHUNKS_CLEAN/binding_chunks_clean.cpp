#pragma once
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <pybind11/complex.h>
#include <pybind11/chrono.h>
#include <pybind11/complex.h>
#include <algorithm>
#include <any>           // só se realmente usar std::any
#include <cstddef>       // para std::size_t (opcional, pois outros headers já puxam)
#include <cstring>
#include <exception>
#include <fstream>
#include <future>        // se usar std::future
#include <iomanip>
#include <iostream>
#include <map>           // só se usar std::map
#include <memory>
#include <optional>      // **<-- aqui**
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>       // só se usar std::variant
#include <vector>
#include <tuple>

#include <cmath>
#include <filesystem>    // se usar
#include <omp.h>
#include <syncstream>

// somente uma vez
#include <torch/torch.h>
#if __has_include(<semaphore>)
    #include <semaphore>
#else
    // Alternativa: inclua uma implementação própria ou de terceiros
    #include "semaphore_compat.h"
#endif
 
#if __has_include(<format>)
    #include <format>
#else
    // Alternativa: use a biblioteca fmt ou implemente uma função de formatação simples
    #include <fmt/core.h>
#endif
 
#include <iostream>
#include <filesystem>
#include <Python.h>
 
// Cabeçalhos do seu projeto
#include "RagException.h"
#include "ThreadSafeQueue.h"
#include "StringUtils.h"
#include "FileUtilsLocal.h"
#include "CommonStructs.h"
 
//#include "ContentCleaner.h"
#include "ContentCleaner.h"
 
#include "ChunkDefault/ChunkDefault.h"
#include "ChunkCount/ChunkCount.h"
#include "ChunkSimilarity/ChunkSimilarity.h"
#include "ChunkCommons/ChunkCommons.h"
#include "ChunkQuery/ChunkQuery.h"
 
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
 
 
//--------------------------------------------------------------------------
// Função de binding para o ChunkCommon
//--------------------------------------------------------------------------
void bind_ChunkCommons(py::module& m)
{
    //--------------------------------------------------------------------------
    // Binding para o enum EmbeddingModel
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
    // Binding para a função MeanPooling
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
    // Binding para a função NormalizeEmbeddings
    //--------------------------------------------------------------------------
    m.def("NormalizeEmbeddings", &Chunk::NormalizeEmbeddings,
        py::arg("embeddings"),
        R"doc(
              Normaliza as embeddings para que tenham norma 1.
              
              Parâmetros:
                  embeddings (list[float]): Lista de embeddings a serem normalizadas.
          )doc");
 
    //--------------------------------------------------------------------------
    // Binding para a função EmbeddingModelBatch
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
    // Binding para a função EmbeddingHuggingFaceTransformers
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
    // Binding para a função EmbeddingOpeanAI
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
    // Binding para a função toTensor
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
    // Binding para a função SplitText
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
    // Binding para a função SplitTextByCount
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
// Binding para ChunkDefault
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
// Binding para Chunk::ChunkSimilarity
// --------------------------------------------------------------------------
 
/**
* Cria o binding da classe ChunkSimilarity, responsável por dividir
* texto em chunks e gerar embeddings para análise de similaridade.
*/
void bind_ChunkSimilarity(py::module& m)
{
    // Primeiro, criamos o binding para a enum EmbeddingModel (se ainda não foi criado).
    // Se preferir, você pode chamar 'bind_EmbeddingModel(m);' dentro do PYBIND11_MODULE,
    // mas aqui pode ficar junto por conveniência.
 
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
// Função de binding para o ChunkQuery
//--------------------------------------------------------------------------
// Precondition: Document binding must be defined before this function
// void bind_RAGLibrary_Document(py::module &m);


void bind_ChunkQuery(py::module& m) {
    py::class_<Chunk::ChunkQuery, std::shared_ptr<Chunk::ChunkQuery>>(m, "ChunkQuery",
        R"doc(
            Class for processing queries over text chunks, generating embeddings,
            and evaluating similarity against a provided query.
        )doc")
        // Construtor
        .def(py::init<
                std::string&,                       // query text
                RAGLibrary::Document&,             // optional document containing content and embedding
                std::vector<RAGLibrary::Document>&,// list of documents for chunk indexing
                const Chunk::EmbeddingModel,       // embedding model enum
                const std::string&                 // model name string
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
       // Sobrecarga Query(Document)
       .def("Query",
            py::overload_cast<RAGLibrary::Document>(&Chunk::ChunkQuery::Query),
            py::arg("query_doc"),
            R"doc(
                Uses the provided Document (with or without embedding) to set the query.
            )doc")

        // Método CreateVD
        .def("CreateVD", &Chunk::ChunkQuery::CreateVD,
             py::arg("chunks_list"),
             R"doc(
                 Creates embeddings for a list of chunks.

                 Parameters:
                     chunks_list (list[RAGLibrary.Document]): Documents for which to generate embeddings.

                 Returns:
                     list[list[float]]: Embedding vectors for each chunk.
             )doc")
        // Método Retrieve (agora retorna tuple(str, float, int))
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
        // Método getRetrieveList (também retorna tuple(str, float, int))
        .def("getRetrieveList", &Chunk::ChunkQuery::getRetrieveList,
             R"doc(
                 Returns the list of retrieved chunks with their similarity scores and original indices.

                 Returns:
                     list[tuple(str, float, int)]: Retrieved chunk content, similarity score, and original index triplets.
             )doc")
        // Método StrQ
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
// Binding para ContentCleaner
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
// Módulo principal
//--------------------------------------------------------------------------
PYBIND11_MODULE(purecpp_chunks_clean, m)
{   m.doc() = "Bindings unificados do RagPUREAI binding_chuncks_clean";
 
    //bind_RagException(m);
    //bind_FileUtilsLocal(m);
    //bind_StringUtils(m);
    //bind_CommonStructs(m);
 
    bind_ContentCleaner(m);
 
    bind_ChunkCommons(m);
    bind_ChunkDefault(m);
    bind_ChunkCount(m);
    bind_ChunkQuery(m);
    bind_ChunkSimilarity(m);
}
 