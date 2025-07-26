// python/_vectordb.cpp
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>          // std::vector, std::unordered_map, …
#include "vectordb/document.h"
#include "vectordb/registry.h"
#include "vectordb/backend.h"

namespace py = pybind11;
using namespace vdb;

PYBIND11_MODULE(vectordb, m) {
    m.doc() = "Python bindings para VectorDatabase (PureCPP)";

    /* ------------------------ Document ----------------------- */
    py::class_<Document>(m, "Document")
        .def(py::init<std::string, std::vector<float>,
                      std::unordered_map<std::string, std::string>>(),
             py::arg("page_content"),
             py::arg("embedding"),
             py::arg("metadata") = std::unordered_map<std::string,std::string>())
        .def_readwrite("page_content", &Document::page_content)
        .def_readwrite("embedding",    &Document::embedding)
        .def_readwrite("metadata",     &Document::metadata)
        .def("__len__", &Document::dim)
        .def("__repr__", [](const Document& d){
            return "<Document dim=" + std::to_string(d.dim()) + ">";
        });

    /* ------------------------ VectorBackend ------------------ */
    py::class_<VectorBackend, std::shared_ptr<VectorBackend>>(m, "VectorBackend")
        .def("insert",  &VectorBackend::insert,
             py::arg("docs"),
             "Insere uma lista de Document")
        .def("query",   &VectorBackend::query,
             py::arg("embedding"), py::arg("k") = 5,
             py::arg("filter") = nullptr,
             "Retorna top-k (doc, score)");

    /* ----------------------- Factory ------------------------- */
    m.def("make_backend",
          [](const std::string& name,
             const std::string& json_cfg) -> std::shared_ptr<VectorBackend>
          {
              auto cfg = nlohmann::json::parse(json_cfg);
              return Registry::instance().make(name, cfg);
          },
          py::arg("name"), py::arg("json_cfg"),
          R"pbdoc(
              Instancia o backend solicitado.
              Exemplo:
                  store = vectordb.make_backend(
                      "redis",
                      '{"dim": 768, "uri": "tcp://localhost:6379"}'
                  )
          )pbdoc");

    /* ----------------------- Utilidades ---------------------- */
    m.def("list_backends", []{
        return Registry::instance().list();
    });
}
