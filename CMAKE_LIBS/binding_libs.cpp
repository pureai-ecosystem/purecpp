 
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
#include <re2/re2.h>
#include <semaphore>
#include <format>
#include <iostream>
#include <filesystem>
#include <Python.h>
// #include <torch/torch.h>
//LIBS
#include "RagException.h"
#include "ThreadSafeQueue.h"
#include "StringUtils.h"
#include "FileUtilsLocal.h"
#include "CommonStructs.h"
 
 
namespace py = pybind11;
using namespace RAGLibrary;
// using namespace DataLoader;
 
typedef std::vector<std::pair<std::string, std::string>> test_vec_pair;
// using   test_vec_pair = std::vector<std::pair<std::string, std::string>>;
 
// Não use `using namespace PDFLoader;` para evitar conflitos.
// A classe será referenciada usando qualificação total.
 
//--------------------------------------------------------------------------
// Função de binding para RagException
//--------------------------------------------------------------------------
void bind_RagException(py::module& m)
{
    py::register_exception<RagException>(m, "RagException");
}
 
// --------------------------------------------------------------------------
// Binding para FileUtils
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
// Binding para StringUtils
// --------------------------------------------------------------------------
void bind_StringUtils(py::module& m)
{
    m.def("escapeRegex", &StringUtils::escapeRegex, py::arg("str"),
        "Escapa caracteres especiais em uma string para uso em expressões regulares.");
 
    // Para joinStr, criamos um pequeno lambda para retornar a string resultante
    // já que a função original preenche via referência.
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
// Template para ThreadSafeQueue
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
// Função de binding para as structs comuns do namespace RAGLibrary
// --------------------------------------------------------------------------
 
 
void bind_CommonStructs(py::module& m)
{
    // ----------------------------------------------------------------------
    // DataExtractRequestStruct
    // ----------------------------------------------------------------------
    // py::class_<RAGLibrary::DataExtractRequestStruct>(m, "DataExtractRequestStruct",
    //                                                  R"doc(
    //         Estrutura que armazena informações sobre o caminho do arquivo (targetIdentifier)
    //         e um limitador de conteúdo (extractContentLimit) para extração.
    //     )doc")
    //     .def(py::init<>(),
    //          R"doc(
    //              Construtor padrão, inicializa targetIdentifier como string vazia
    //              e extractContentLimit como 0.
    //          )doc")
    //     .def(py::init<const std::string &, unsigned int>(),
    //          py::arg("targetIdentifier"),
    //          py::arg("extractContentLimit") = 0,
    //          R"doc(
    //              Construtor que recebe um identificador de arquivo (targetIdentifier)
    //              e um limite opcional de conteúdo (extractContentLimit).
    //          )doc")
    //     .def_readwrite("targetIdentifier", &RAGLibrary::DataExtractRequestStruct::targetIdentifier,
    //                    "Identificador do arquivo (caminho ou nome).")
    //     .def_readwrite("extractContentLimit", &RAGLibrary::DataExtractRequestStruct::extractContentLimit,
    //                    "Limite máximo de páginas/linhas a serem extraídas (0 = sem limite).");
    py::class_<RAGLibrary::LoaderDataStruct>(m, "LoaderDataStruct")
        .def(py::init<const RAGLibrary::Metadata&, const std::vector<std::string> &>(),
            py::arg("metadata"), py::arg("textContent"))
        .def_readwrite("metadata", &RAGLibrary::LoaderDataStruct::metadata)
        .def_readwrite("textContent", &RAGLibrary::LoaderDataStruct::textContent)
        .def("__str__", [](const RAGLibrary::LoaderDataStruct& data)
            {  std::ostringstream o;
                o << data; // Usa o operador << sobrecarregado
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
                // Usa o operador << friend definido na struct
                std::ostringstream oss;
                oss << data;
                return oss.str(); });
 
    // ----------------------------------------------------------------------
    // Document (exposto como 'RAGDocument' em Python)
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
 
PYBIND11_MODULE(purecpp_libs, m)
{   m.doc() = "Bindings unificados do RagPUREAI binding_chuncks_clean";
 
    bind_RagException(m);
    bind_FileUtilsLocal(m);
    bind_StringUtils(m);
    bind_CommonStructs(m);
}