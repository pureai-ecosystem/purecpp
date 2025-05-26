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
//#include <torch/torch.h>
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
 
#include "IBaseLoader.h"
#include "BaseLoader.h"
//#include "PDFLoader.h"
#include "PDFLoader/PDFLoader.h"
#include "DOCXLoader/DOCXLoader.h"
#include "TXTLoader/TXTLoader.h"
#include "WebLoader/WebLoader.h"
 
namespace py = pybind11;
using namespace RAGLibrary;
using namespace DataLoader;
 
typedef std::vector<std::pair<std::string, std::string>> test_vec_pair;
// using   test_vec_pair = std::vector<std::pair<std::string, std::string>>;
 
 
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
// Classe trampolim para BaseDataLoader
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
// Bind para PDFLoader
//--------------------------------------------------------------------------
// Note: Estamos usando qualificação total para a classe PDFLoader
// Remover o final do método InsertDataToExtract em PDFLoader.h se ainda tiver problemas.
void bind_PDFLoader(py::module& m)
{
    py::class_<::PDFLoader::PDFLoader, std::shared_ptr<::PDFLoader::PDFLoader>, DataLoader::BaseDataLoader>(m, "PDFLoader")
        .def(py::init<const std::string, const unsigned int &>(),
            py::arg("filePath"),
            py::arg("numThreads") = 1,
            "Creates a PDFLoader with a file path and an optional number of threads.");
}
// A função bind_DOCXLoader é similar à bind_PDFLoader
void bind_DOCXLoader(py::module& m)
{
    py::class_<::DOCXLoader::DOCXLoader, std::shared_ptr<::DOCXLoader::DOCXLoader>, DataLoader::BaseDataLoader>(m, "DOCXLoader")
        .def(py::init<const std::string, const unsigned int &>(),
            py::arg("filePath"),
            py::arg("numThreads") = 1,
            "Creates a DOCXLoader with a file path and an optional number of threads.");
}
 
// Função de binding para TXTLoader
void bind_TXTLoader(py::module& m)
{
    py::class_<::TXTLoader::TXTLoader, std::shared_ptr<::TXTLoader::TXTLoader>, DataLoader::BaseDataLoader>(m, "TXTLoader")
        .def(py::init<const std::string, const unsigned int &>(),
            py::arg("filePath"),
            py::arg("numThreads") = 1,
            "Creates a TXTLoader, optionally with initial paths and a defined number of threads.");
}
 
// Função de binding para WebLoader
void bind_WebLoader(py::module& m)
{
    py::class_<::WebLoader::WebLoader, std::shared_ptr<::WebLoader::WebLoader>, DataLoader::BaseDataLoader>(m, "WebLoader")
        .def(py::init<const std::string, const unsigned int &>(),
            py::arg("url"),
            py::arg("numThreads") = 1,
            "Creates a WebLoader with optional URLs and a defined number of threads.");
}
 
 
typedef std::vector<std::pair<std::string, std::string>> test_vec_pair;
// using   test_vec_pair = std::vector<std::pair<std::string, std::string>>;
 
// Não use `using namespace PDFLoader;` para evitar conflitos.
// A classe será referenciada usando qualificação total.
 

//--------------------------------------------------------------------------
// Módulo principal
//--------------------------------------------------------------------------
PYBIND11_MODULE(purecpp_extract, m)
{   m.doc() = "Bindings unificados do purecpp_extract";
 
   
    bind_IBaseDataLoader(m);
    bind_BaseDataLoader(m);
    bind_PDFLoader(m);
    bind_DOCXLoader(m);
    bind_TXTLoader(m);
    bind_WebLoader(m);
}