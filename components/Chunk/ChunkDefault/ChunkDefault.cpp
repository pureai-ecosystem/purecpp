
#include "ChunkDefault.h"
#include "RagException.h"
#include "StringUtils.h"
#include <cmath>
#include <omp.h>
#include <syncstream>
#include <cstring>
#include <iostream>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <memory>     
#include <sstream>     
#include <torch/torch.h>
#include <iomanip>    
#include <stdexcept>
// using namespace Chunk;

Chunk::ChunkDefault::ChunkDefault(
    const int chunk_size, 
    const int overlap, 
    std::optional<std::vector<RAGLibrary::Document>> items_opt, 
    int max_workers): 
    m_chunk_size(chunk_size), m_overlap(overlap)
{
    if (m_overlap >= m_chunk_size) {
        throw RAGLibrary::RagException("The overlap value must be smaller than the chunk size.");
    }
    if (items_opt.has_value())
    ProcessDocuments(*items_opt, max_workers);
}  

const Chunk::vdb_data& Chunk::ChunkDefault::CreateEmb(std::string model){
    // Validation of input parameters ------------------------- 
    Chunk::to_lowercase(model);

    if (this->chunks.empty())
        throw std::invalid_argument("Empty chunks list.");
    
    std::optional<std::string> vendor_opt = resolve_vendor_from_model(model);

    if(vendor_opt==std::nullopt)
        throw std::invalid_argument("Model not supported.");

    if(is_this_model_used_yet(model))
        throw std::invalid_argument("There is already an element of this chunk like this.");
    // ---------------------------------------------------------
    std::vector<RAGLibrary::Document> docs;

    try{
        docs = Embeddings(this->chunks, model);
    }
    catch (const std::exception& e) {
        std::cerr << "[Exception] " << e.what() << "\n";
        throw;
    }  
    //============================================================
    Chunk::vdb_data vdb_element;
    
    vdb_element.dim = docs[0].embedding->size();
    vdb_element.n = this->chunks.size();

    // 1. Gerar flatVD ... E apartir daqui vai...
    vdb_element.flatVD.clear();
    vdb_element.flatVD.reserve(vdb_element.n * vdb_element.dim);
    for (const auto& doc : docs) {
        if (!doc.embedding.has_value() || doc.embedding->size() != vdb_element.dim)
            throw std::runtime_error("Missing or inconsistent embedding.");
        vdb_element.flatVD.insert(vdb_element.flatVD.end(), doc.embedding->begin(), doc.embedding->end());
    }

    vdb_element.model = model;
    vdb_element.vendor = vendor_opt.value();

    const size_t expected_size = vdb_element.n * vdb_element.dim;

    
    std::cout << "Flatten vector dimensions: <" << vdb_element.flatVD.size() << ">\n";
    std::cout << "dim: " << vdb_element.dim << " n: " << vdb_element.n << " → expected size: " << expected_size << "\n";
    std::cout << "Model " << vdb_element.model << ", " << vdb_element.vendor << "\n";

    if (vdb_element.flatVD.size() != expected_size) {
        throw std::runtime_error("Flattened vector has unexpected size.");
    }

    // O objetivo é só criar uma referencia diminuindo a copia dos embeddings 
    // resefencia (se tiver aos alem dos elementos, tb aos embeddings )
    this->elements.push_back(vdb_element);
    const auto& last = this->elements.back();
    //RAGLibrary::print_memory();
    std::cout << "╔═════════════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║ ➤ Model: " << last.model << " was added to chunks                      \n";
    std::cout << "╚═════════════════════════════════════════════════════════════════════════════════════╝\n";
    
    LogEmbeddingStats(last.model, last.vendor, last.dim, last.n, last.flatVD.size());

    return last;
}

std::vector<RAGLibrary::Document> Chunk::ChunkDefault::ProcessSingleDocument(RAGLibrary::Document &item)
{
    std::vector<RAGLibrary::Document> documents;
    try
    {
        auto chunks = Chunk::SplitText(item.page_content, m_overlap, m_chunk_size);
        documents.reserve(documents.size() + chunks.size());
        for (auto &chunk : chunks)
        {
            documents.push_back(RAGLibrary::Document(item.metadata, chunk));
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        throw;
    }

    return documents;
}

const std::vector<RAGLibrary::Document>& Chunk::ChunkDefault::ProcessDocuments(std::optional<std::vector<RAGLibrary::Document>> items_opt, int max_workers)
{   
    if (this->initialized_)
        throw std::invalid_argument("Chunks list already initialized.");

    
    if (!items_opt.has_value() || items_opt->empty()) {
        throw std::invalid_argument("No documents provided in items_opt.");
    }
    // Acesso direto sem cópia
    const auto& items = *items_opt;

    this->metadata = items[0].metadata;

    std::vector<RAGLibrary::Document> documents;
    
    try
    {
        int max_threads = omp_get_max_threads();
        if (max_workers > 0 && max_workers < max_threads)
        {
            max_threads = max_workers;
        }

        omp_set_num_threads(max_threads);
#pragma omp parallel for
        for (int i = 0; i < items.size(); i++)
        {
            auto &item = items[i];
            auto chunks = Chunk::SplitText(item.page_content, m_overlap, m_chunk_size);

#pragma omp critical
            {
                documents.reserve(documents.size() + chunks.size());
                for (auto &chunk : chunks)
                {
                    documents.push_back(RAGLibrary::Document({}, chunk));
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        throw;
    }

    this->initialized_ = true;
    this->chunks = documents;

    return this->chunks;
}
//PRINTS======================================================================================
void Chunk::ChunkDefault::LogEmbeddingStats(std::string model, std::string vendor , size_t dim, size_t n, size_t flatVD_size) const{
    std::cout << "╔═════════════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║ ➤ Model / Vendor    : " << model << " / " << vendor << "         \n";
    std::cout << "║ ➤ Dimensions      : " << dim << "         \n";
    std::cout << "║ ➤ Flat Vector Size: " << flatVD_size << "         \n";
    std::cout << "╚═════════════════════════════════════════════════════════════════════════════════════╝\n";
}

void Chunk::ChunkDefault::printVD(void) {
    if (this->elements.size()==0) {
        std::cout << "This chunk elements is not available.\n";
        return;
    }

    size_t total_chunks = chunks.size();
    size_t total_embeddings = this->elements.size();

    if (total_embeddings > 0){
        std::cout << "Chunk list contains " << total_chunks << " chunks.\n";
        std::cout << "With an average size of: "<< m_chunk_size
        << " and  overlap of: " << m_overlap 
        << " | Quantity of different embeddings: " << total_embeddings  << "\n";
        for (size_t i = 0; i < total_embeddings; ++i) {
            LogEmbeddingStats(this->elements[i].model, this->elements[i].vendor, this->elements[i].dim, this->elements[i].n, this->elements[i].flatVD.size());
        }
        return;
    }
}
//GETS==========================================================================================

const std::vector<RAGLibrary::Document>& Chunk::ChunkDefault::getChunks() const {
    if (this->chunks.empty()) {
        throw std::runtime_error("Chunks is empty");
    }
    return this->chunks;
}

const Chunk::vdb_data* Chunk::ChunkDefault::getElement(size_t pos) const{
    if (pos < this->elements.size())
        return &this->elements[pos];
    return nullptr;
}

//================================================================================================
size_t Chunk::ChunkDefault::quant_of_elements(void) const {
    return this->elements.size();
}

void Chunk::ChunkDefault::clear(void) {
    chunks.clear();
    this->elements.clear();
    initialized_ = false;
}