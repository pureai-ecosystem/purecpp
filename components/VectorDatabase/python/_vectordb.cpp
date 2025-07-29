#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

#include <nlohmann/json.hpp>

#include "vectordb/document.h"
#include "vectordb/backend.h"
#include "vectordb/registry.h"

#include <cstring>

namespace py = pybind11;
using namespace vdb;

static std::vector<float> to_vecf(const py::handle& obj) {
    if (py::isinstance<py::array>(obj)) {
        py::array arr = py::cast<py::array>(obj);
        if (arr.ndim() != 1) throw std::runtime_error("The embedding should be 1D.");
        std::vector<float> v(arr.size());
        std::memcpy(v.data(), arr.data(), arr.size() * sizeof(float));
        return v;
    }
    return py::cast<std::vector<float>>(obj);
}

PYBIND11_MODULE(vectordb, m) {
    m.doc() = "Python bindings for VectorDatabase (PureCPP)";

    
    py::class_<Document>(m, "Document")
        .def(py::init<std::string,
                      std::vector<float>,
                      std::unordered_map<std::string,std::string>>(),
             py::arg("page_content"),
             py::arg("embedding"),
             py::arg("metadata") = std::unordered_map<std::string,std::string>{})
        .def_property_readonly("dim", &Document::dim)
        .def_readwrite("page_content", &Document::page_content)
        .def_readwrite("embedding",    &Document::embedding)
        .def_readwrite("metadata",     &Document::metadata)
        .def("to_json",   &Document::to_json)
        .def_static("from_json", [](const std::string& s){ return Document::from_json(s); })
        .def("__repr__", [](const Document& d){
            return "<Document dim=" + std::to_string(d.dim()) + ">";
        });


    py::class_<QueryResult>(m, "QueryResult")
        .def_readonly("doc",   &QueryResult::doc)    
        .def_readonly("score", &QueryResult::score)
        .def("__repr__", [](const QueryResult& r){
            return "<QueryResult score=" + std::to_string(r.score) + ">";
        });

    
    py::class_<VectorBackend, std::shared_ptr<VectorBackend>>(m, "VectorBackend")
        .def("insert", [](VectorBackend& self, py::object py_docs){
            std::vector<Document> docs;
            if (py::isinstance<py::list>(py_docs)) {
                for (py::handle h : py_docs) docs.push_back(py::cast<Document>(h));
            } else {
                docs.push_back(py::cast<Document>(py_docs));
            }
            self.insert(docs);
        }, py::arg("docs"), "Inserts one or a list of Documents..")
        .def("query", [](VectorBackend& self,
                         py::object embedding,
                         std::size_t k,
                         py::object filt_obj){
            auto emb = to_vecf(embedding);

            std::unordered_map<std::string,std::string> filt;
            const std::unordered_map<std::string,std::string>* pf = nullptr;
            if (!filt_obj.is_none()) {
                filt = py::cast<std::unordered_map<std::string,std::string>>(filt_obj);
                pf = &filt;
            }
            return self.query(emb, k, pf);
        }, py::arg("embedding"), py::arg("k") = 5, py::arg("filter") = py::none(),
           "Executes a KNN search and returns a list of QueryResult.")
        .def("is_open", &VectorBackend::is_open)
        .def("close",   &VectorBackend::close)
        .def("__repr__", [](const VectorBackend&){
            return "<VectorBackend>";
        });

    
    m.def("make_backend",
          [](const std::string& name, const std::string& json_cfg){
              auto cfg = nlohmann::json::parse(json_cfg);
              return Registry::instance().make(name, cfg);
          },
          py::arg("name"), py::arg("json_cfg"),
          "Instantiates a registered backend from a configuration JSON.");

    m.def("list_backends", []{
        return Registry::instance().list();
    }, "Lists the registered backends..");
}
