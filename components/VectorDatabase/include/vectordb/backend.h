#pragma once
#include "document.h"
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace vdb {

struct QueryResult { Document doc; float score; };

class VectorBackend {
public:
    explicit VectorBackend(std::uint32_t dim) : dim_(dim) {}
    virtual ~VectorBackend()                           = default;

    std::uint32_t dim() const noexcept { return dim_; }
    virtual bool  is_open() const noexcept            = 0;

    virtual void insert(std::span<const Document> docs)            = 0;
    virtual std::vector<QueryResult>
           query(std::span<const float> embedding, std::size_t k,
                 const std::unordered_map<std::string,std::string>* filter=nullptr) = 0;

    // opcionais
    virtual void close() {}
protected:
    std::uint32_t dim_;
};

using VectorBackendPtr = std::unique_ptr<VectorBackend>;

} // namespace vdb
