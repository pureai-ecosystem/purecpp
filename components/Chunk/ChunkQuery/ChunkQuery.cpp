
#include "ChunkQuery.h"
#include "RagException.h"
#include "StringUtils.h"
#include <cstring>
#include <iostream>
#include <cmath>
#include <fstream>
#include <omp.h>
#include <syncstream>
#include <algorithm>
#include <memory>      // unique_ptr, make_unique
#include <sstream>     // stringstream
#include <torch/torch.h>
#include <iomanip>    // setprecision
#include <stdexcept>  // std::invalid_argument

// #define DEBUG // Uncomment to enable debug messages

Chunk::ChunkQuery::ChunkQuery(
        std::string query,
        RAGLibrary::Document query_doc,
        const Chunk::ChunkDefault* chunks,
        std::optional<size_t> pos,
        float threshold
): m_query(std::move(query)), m_query_doc(std::move(query_doc))
{
    if (pos.has_value() && chunks != nullptr) {
        try{
            setChunks(*chunks, pos.value()); 
        }
        catch(const std::exception& e){
            throw;
        }
    }
    // else cout<<"Invalid";

    if (!query.empty()) {
        Query(query);
    } 
    else if (!query_doc.page_content.empty()) {
        Query(query_doc);
    }
}

void Chunk::ChunkQuery::setChunks(const Chunk::ChunkDefault& chunks, size_t pos) {
    if (!chunks.isInitialized())
        throw std::invalid_argument("No class.");

    size_t x = chunks.quant_of_elements();
    if (x == 0)
        throw std::invalid_argument("Empty chunks vector.");
        
    if (pos >= x)
        throw std::out_of_range("Position out of range.");

    const auto* vdb = chunks.getElement(pos);
    if (!vdb)
        throw std::invalid_argument("No element in this position.");

    if (m_vdb && m_vdb->model != vdb->model && !m_query.empty()) {
        m_emb_query.clear();
        auto docs = Chunk::Embeddings({ RAGLibrary::Document({}, m_query)}, vdb->model);
        if (!docs[0].embedding.has_value())
            throw std::runtime_error("Missing embedding in generated doc.");
        m_query_doc = docs[0];
        m_emb_query = docs[0].embedding.value();
    }

    //------------------------------------------------------------------------------------------------------
    //Criar views via span
    if (!m_chunk_embedding.empty()) m_chunk_embedding.clear();
    m_vdb = vdb; 
    m_chunk_embedding.reserve(m_vdb->n);
    for (size_t i = 0; i < m_vdb->n; ++i) {
        const float* ptr = m_vdb->flatVD.data() + (i * m_vdb->dim);
        m_chunk_embedding.emplace_back(ptr, m_vdb->dim); 
    }
    if (m_chunk_embedding.empty()) throw std::runtime_error("Unable to create window");
    //------------------------------------------------------------------------------------------------------
    m_n_chunk =vdb->n;
    m_dim = vdb->dim;
    m_pos = pos;
    m_chunks_list = &chunks.getChunks(); 
    m_chunks = &chunks;
    m_query_doc = {};  //clear
    m_emb_query.clear();

    RAGLibrary::Document result;
    if(m_vdb!=nullptr){
        try{
            auto results = Chunk::Embeddings({ RAGLibrary::Document({}, m_query) }, m_vdb->model);// sempre retorna algo 
            result = validateEmbeddingResult(results);
        }
        catch(const std::exception& e){
            throw;
        }
        m_emb_query = result.embedding.value(); 
        m_n = 1;
        m_query_doc = result;
        m_query_doc.metadata["model"] = m_vdb->model;
    }
}

RAGLibrary::Document Chunk::ChunkQuery::Query(std::string query, const Chunk::ChunkDefault* temp_chunks, std::optional<size_t> pos){
    if (query.empty() || query.size()<5) {
        throw std::invalid_argument("Query string is empty.");
    }
    if (pos.has_value()){
        if (temp_chunks != nullptr) setChunks(*temp_chunks, pos.value());
        else if(m_chunks != nullptr) setChunks(*m_chunks, pos.value());
    }

    m_query_doc = {};  //clear
    m_emb_query.clear();
    m_query = query;
    RAGLibrary::Document result;
    if(m_vdb!=nullptr){
        //std::vector<RAGLibrary::Document> results = Chunk::Embeddings({ RAGLibrary::Document({}, query) }, vdb->model)[0];
        try{
            auto results = Chunk::Embeddings({ RAGLibrary::Document({}, query) }, m_vdb->model);// sempre retorna algo 
            result =validateEmbeddingResult(results);
        }
        catch(const std::exception& e){
            throw;
        }
        m_emb_query = result.embedding.value(); 
        m_n = 1;
        m_query_doc = result;
        m_query_doc.metadata["model"] = m_vdb->model;
    }
    else {
        m_query_doc = RAGLibrary::Document({}, query);
        m_n = 0;
    }

    return m_query_doc;
}


// Versão que recebe diretamente um Document
RAGLibrary::Document Chunk::ChunkQuery::Query(RAGLibrary::Document query_doc, const Chunk::ChunkDefault* temp_chunks, std::optional<size_t> pos) {
    if (query_doc.page_content.empty()) {
        throw std::invalid_argument("Query document is empty.");
    }

    if (pos.has_value()){
        this->m_query = query_doc.page_content;
        if (temp_chunks != nullptr) setChunks(*temp_chunks, pos.value());
        else if(m_chunks != nullptr) setChunks(*m_chunks, pos.value());
    }

    RAGLibrary::Document result;
    if(this->m_vdb!=nullptr){
        //-----------------------------------------
        this->m_query_doc = {};  
        this->m_emb_query.clear();
        //-----------------------------------------
        bool needs_embedding = !query_doc.embedding.has_value() || query_doc.embedding->empty();

        bool wrong_model = true;
        auto it = query_doc.metadata.find("model");
        if (it != query_doc.metadata.end()) {
            wrong_model = (it->second != m_vdb->model);
        }

        if (needs_embedding || wrong_model) {
            auto results = Chunk::Embeddings({ query_doc }, m_vdb->model);
            result = validateEmbeddingResult(results);
            this->m_query_doc = result;
            this->m_query_doc.metadata["model"] = m_vdb->model;
            m_n = 1;
        } else {
            this->m_query_doc = query_doc;
        }
        this->m_emb_query = this->m_query_doc.embedding.value();
        m_n = 1;
    }
    m_n = 1;
    return this->m_query_doc;
}
//======================================================================================================
std::vector<std::tuple<std::string, float, int>> Chunk::ChunkQuery::Retrieve(float threshold, const Chunk::ChunkDefault* temp_chunks, std::optional<size_t> pos) {
    // Validation of input parameters -----------------------------------------------------------------------
    if (m_emb_query.empty()) throw std::runtime_error("Query not yet initialized.");
    if (threshold < -1.0f || threshold > 1.0f) throw std::invalid_argument("Threshold out of bound [-1,1].");
    if (m_vdb->flatVD.empty()) throw std::runtime_error("Embeddings not found.");
    if (pos.has_value()){
        if (temp_chunks != nullptr) setChunks(*temp_chunks, pos.value());
        else if(m_chunks != nullptr) setChunks(*m_chunks, pos.value());
        else throw std::invalid_argument("Position was provided, but no chunk context (temp_chunks or m_chunks) was set.");
    }

    // Vetor temporário de (texto, score, índice)
    std::vector<std::tuple<std::string, float, int>> scored_hits;
    // Tensor da query
    auto query_tensor = torch::from_blob(
        const_cast<float*>(m_emb_query.data()),
        {int64_t(m_emb_query.size())}, torch::kFloat32
    );
    // auto query_tensor = torch::tensor(m_emb_query, torch::kFloat32); -> ?

    float norm_q = torch::norm(query_tensor).item<float>();

    #pragma omp parallel
    {
        std::vector<std::tuple<std::string, float, int>> local_hits;
        #pragma omp for nowait
        for (int i = 0; i < int(m_chunk_embedding.size()); ++i) {
            auto& emb = m_chunk_embedding[i];
            auto chunk_tensor = torch::from_blob(
                const_cast<float*>(emb.data()),
                {int64_t(emb.size())}, torch::kFloat32
            );

            float norm_c = torch::norm(chunk_tensor).item<float>();
            float dot_p = torch::dot(query_tensor, chunk_tensor).item<float>();
            float sim = dot_p / (norm_q * norm_c);

            if (sim >= threshold) {// armazena (conteúdo, similaridade, índice original)
                local_hits.emplace_back(
                    //const auto& doc = (*this->chunks_list)[i];
                    //std::cout << doc.page_content << std::endl;,
                    (*this->m_chunks_list)[i].page_content,
                    sim,
                    i
                );
            }
        }
        #pragma omp critical
        scored_hits.insert(
            scored_hits.end(),
            local_hits.begin(),
            local_hits.end()
        );
    }

    // ordena decrescentemente por similaridade (get<1>)
    std::sort(
        scored_hits.begin(),
        scored_hits.end(),
        [](auto &a, auto &b) {
            return std::get<1>(a) > std::get<1>(b);
        }
    );

    // atualiza estado e retorna
    m_retrieve_list   = std::move(scored_hits);
    quant_retrieve_list = int(m_retrieve_list.size()); //int quant_retrieve_list = static_cast<int>(m_retrieve_list.size());
    return m_retrieve_list;
}

// Formated P ===========================================================================================

std::string Chunk::ChunkQuery::StrQ(int index) {
    if (index == -1) {
        index = quant_retrieve_list; // usa o valor interno aqui
    }
    const int n = static_cast<int>(m_retrieve_list.size());
    if (index < 0 || index >= n) {
        throw std::out_of_range("Index is out of bounds in the retrieved chunks list.");
    }
    std::ostringstream relevant_context;
    for (int i = 0; i < index; ++i) {
        const auto& [txt_i, score_i, idx_i] = m_retrieve_list[i];
        relevant_context
            << i << ". Score [" << score_i << "] "
            << txt_i << "\n";
    }
    std::ostringstream ss;
    ss << "### Question:\n"
       << m_query << "\n\n"
       << "### Relevant context:\n"
       << relevant_context.str() << "\n"
       << "Based on this context, provide a precise, concise, and well-reasoned answer. "
          "Answer in the language of the question.\n";

    return ss.str();
}
//========================================================================================================

// Getters ===============================================================================================
RAGLibrary::Document Chunk::ChunkQuery::getQuery(void) const {
    return this->m_query_doc;
}

const std::vector<RAGLibrary::Document>& Chunk::ChunkQuery::getChunksList() const {
    if (m_chunks_list == nullptr) {
        throw std::runtime_error("Chunks list not initialized.");
    }
    return *m_chunks_list;
}

// Retorna o par (modelo de embedding, nome do modelo) usados na instância
std::string Chunk::ChunkQuery::getMod(void) const {
    return { this->m_vdb->model };
}

std::tuple<size_t, size_t, size_t> Chunk::ChunkQuery::getPar(void) const {
    return { this->m_n, this->m_dim, this->m_n_chunk };
}

std::vector<float> Chunk::ChunkQuery::getEmbedQuery(void) const {
    return m_emb_query;
}

std::vector<std::tuple<std::string, float, int>> Chunk::ChunkQuery::getRetrieveList(void) const {
    if(this->m_retrieve_list.size() == 0)
        std::cout<<"Empty Retrive List\n";
        return {};
    return this->m_retrieve_list;
}
//======================================================================================================