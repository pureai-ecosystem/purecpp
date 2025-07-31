#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <nlohmann/json.hpp>

#include "vectordb/backend.h"
#include "vectordb/registry.h"
#include "CommonStructs.h"

#include <cstring>

namespace py = pybind11;
namespace vdb
{
    void force_link_redis_backend(); // declaração
}
static std::vector<float> to_vecf(const py::handle &obj)
{
    if (py::isinstance<py::array>(obj))
    {
        py::array arr = py::cast<py::array>(obj);
        if (arr.ndim() != 1)
            throw std::runtime_error("The embedding should be 1D.");
        std::vector<float> v(arr.size());
        std::memcpy(v.data(), arr.data(), arr.size() * sizeof(float));
        return v;
    }
    return py::cast<std::vector<float>>(obj);
}

void bind_VectorDB(py::module_ &m)
{
    vdb::force_link_redis_backend();

    py::class_<QueryResult>(m, "QueryResult")
        .def_readonly("doc", &QueryResult::doc)
        .def_readonly("score", &QueryResult::score)
        .def("__repr__", [](const QueryResult &r)
             { return "<QueryResult score=" + std::to_string(r.score) + ">"; });

    py::class_<VectorBackend, std::shared_ptr<VectorBackend>>(m, "VectorBackend")
        .def("insert", [](VectorBackend &self, py::object py_docs)
             {
            std::vector<RAGLibrary::Document> docs;
            if (py::isinstance<py::list>(py_docs)) {
                for (py::handle h : py_docs) docs.push_back(py::cast<RAGLibrary::Document>(h));
            } else {
                docs.push_back(py::cast<RAGLibrary::Document>(py_docs));
            }
            self.insert(docs); }, py::arg("docs"))
        .def("query", [](VectorBackend &self, py::object embedding, std::size_t k, py::object filt_obj)
             {
            auto emb = to_vecf(embedding);
            std::unordered_map<std::string,std::string> filt;
            const std::unordered_map<std::string,std::string>* pf = nullptr;
            if (!filt_obj.is_none()) {
                filt = py::cast<std::unordered_map<std::string,std::string>>(filt_obj);
                pf = &filt;
            }
            return self.query(emb, k, pf); }, py::arg("embedding"), py::arg("k") = 5, py::arg("filter") = py::none())
        .def("is_open", &VectorBackend::is_open)
        .def("close", &VectorBackend::close)
        .def("__repr__", [](const VectorBackend &)
             { return "<VectorBackend>"; });

    m.def("make_backend", [](const std::string &name, const std::string &json_cfg)
          {
        auto cfg = nlohmann::json::parse(json_cfg);
        return Registry::instance().make(name, cfg); }, py::arg("name"), py::arg("json_cfg"));

    m.def("list_backends", []
          { return Registry::instance().list(); });
}
