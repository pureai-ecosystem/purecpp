#include "ChunkCommons/ChunkCommons.h"
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
#include <memory>
#include <sstream>
#include <torch/torch.h>
#include <iomanip> 
#include <stdexcept> 


Chunk::ChunkQuery::ChunkQuery(
    std::string query,
    RAGLibrary::Document query_doc,
    std::vector<RAGLibrary::Document> chunks_list,
    Chunk::EmbeddingModel embedding_model,
    std::string model
) :  
 m_embedding_model(embedding_model), 
 m_model(std::move(model)),
 m_retrieve_list(),
 quant_retrieve_list(0)
 {

    if (!query.empty() && !query_doc.page_content.empty()) {
        if (query != query_doc.page_content)
            throw std::invalid_argument("Ambiguous input: both query and query_doc differ.");

        m_query = query_doc.page_content;

        if (query_doc.embedding.has_value() && !query_doc.embedding->empty()) {
            m_query_doc = query_doc;
            m_emb_query = m_query_doc.embedding.value(); 
            
        } else {
            auto docs = this->Embeddings({query_doc});
            m_query_doc = docs[0];
            m_emb_query = docs[0].embedding.value(); 
          
        }
    }
    else if (!query.empty() && query_doc.page_content.empty()){   
        m_query = query;
        std::vector<RAGLibrary::Document> list = { RAGLibrary::Document({}, query) };
        auto docs = this->Embeddings(list);
        m_query_doc = docs[0];
        m_emb_query = docs[0].embedding.value();
    }
    else if (query.empty() && !query_doc.page_content.empty()) {
        m_query = query_doc.page_content;
    
        if (!query_doc.embedding.has_value() || query_doc.embedding->empty()) {
            auto docs = this->Embeddings({ query_doc });
            m_query_doc = docs[0];
            m_emb_query = docs[0].embedding.value();
        }
        else {
            m_query_doc = query_doc;
            m_emb_query = query_doc.embedding.value();
        }
    }
    else {
        m_query = "";
        m_query_doc = RAGLibrary::Document({}, query);
        m_emb_query.clear();
    }

    if (!chunks_list.empty()) this->CreateVD(chunks_list);
}

std::vector<RAGLibrary::Document> Chunk::ChunkQuery::Embeddings(const std::vector<RAGLibrary::Document>& list)
{
    if (m_embedding_model !=  Chunk::EmbeddingModel::OpenAI){
        throw std::invalid_argument("Model not yet supported");
    }
    InitAPIKey();
    #ifdef DEBUG
    std::cerr<< "Chave da API do OpenAI: " << m_openai_api_key << std::endl; //Debug
    #endif
    std::vector<RAGLibrary::Document> emb;
    switch (this->m_embedding_model)
    {
    case  Chunk::EmbeddingModel::HuggingFace:
        throw std::invalid_argument("Model not yet supported");
    case  Chunk::EmbeddingModel::OpenAI:
        if(m_model == "text-embedding-ada-002"){
            int count = 0;
            do{
                std::unique_ptr<EmbeddingOpenAI::EmbeddingOpenAI> OpenAI = std::make_unique<EmbeddingOpenAI::EmbeddingOpenAI>();
                emb = OpenAI->GenerateEmbeddings(list, m_model);
                count++;
            }while(!this->allChunksHaveEmbeddings(emb) && count < 3);

            if (!this->allChunksHaveEmbeddings(emb)) {
                throw std::runtime_error("Failed to generate valid embeddings after 3 attempts.");
            }
            return emb;
        }
        else{
            throw std::invalid_argument("Model not yet supported");
        }
        break;
    }

    return {};
}

inline bool Chunk::ChunkQuery::allChunksHaveEmbeddings(const std::vector<RAGLibrary::Document>& chunks_list) {
    return std::all_of(
        chunks_list.begin(),
        chunks_list.end(),
        [](const RAGLibrary::Document& doc) {
            return doc.embedding.has_value();
        });
}

void Chunk::ChunkQuery::InitAPIKey() {
    const char* env_key = std::getenv("OPENAI_API_KEY");
    m_openai_api_key = (env_key != nullptr) ? std::string(env_key) : "";
    if (m_openai_api_key.empty()) {
        #ifdef DEBUG
            std::cerr << "Erro: API key not set.\n";
        #endif
        throw std::runtime_error("API key not set. Please set the OPENAI_API_KEY environment variable.");
    }
    #ifdef DEBUG
    std::cerr << "API key set\n";//Debug
    #endif
}

RAGLibrary::Document Chunk::ChunkQuery::Query(std::string query){
    if (query.empty()) {
        throw std::invalid_argument("Query string is empty.");
    }
    m_query = query;
    std::vector<RAGLibrary::Document> list = { RAGLibrary::Document({}, query) };
    m_query_doc = this->Embeddings(list)[0];
    m_emb_query = m_query_doc.embedding.value(); 
    return m_query_doc;
}

RAGLibrary::Document Chunk::ChunkQuery::Query(RAGLibrary::Document query_doc) {
    if (query_doc.page_content.empty()) {
        throw std::invalid_argument("Query document is empty.");
    }
    m_query = query_doc.page_content;
    if (!query_doc.embedding.has_value() || query_doc.embedding->empty()) {
        auto results = this->Embeddings({ query_doc });
        m_query_doc = results[0];
    } else {
        m_query_doc = query_doc;
    }
    m_emb_query = m_query_doc.embedding.value();
    return m_query_doc;
}

std::vector<std::vector<float>> Chunk::ChunkQuery::CreateVD(std::vector<RAGLibrary::Document> chunks_list)// Testar
{
    if (initialized_)
        throw std::invalid_argument("Chunks list already initialized.");

    if (chunks_list.empty())
        throw std::invalid_argument("Empty chunks list.");

    std::vector<RAGLibrary::Document> docs;
    if (allChunksHaveEmbeddings(chunks_list)) {
        docs = std::move(chunks_list);
    } else {
        docs = this->Embeddings(chunks_list);
    }

    m_chunks_list    = std::move(docs);
    m_chunk_embedding.resize(m_chunks_list.size());

    for (size_t i = 0; i < m_chunks_list.size(); ++i) {
        m_chunk_embedding[i] = m_chunks_list[i].embedding.value();
    }

    initialized_ = true;
    return m_chunk_embedding;
}


void Chunk::ChunkQuery::printVD(int limit) {
    if (m_chunks_list.empty()) {
        #ifdef DEBUG
        std::cerr << "The chunklist is empty\n";
        #endif
        return;
    }
    limit = std::min<int>(limit, static_cast<int>(m_chunks_list.size()));

    for (int i = 0; i < limit; ++i) {
        std::cout << "Chunk[" << i << "]: "
                  << m_chunks_list[i].page_content
                  << "\nEmbedding: m_chunk_embedding[i]"
                  << m_chunk_embedding[i]
                  << "\nEmbedding: m_chunks_list[i].embedding.value()"
                  << m_chunks_list[i].embedding.value()
                  << "\n\n";
    }
}


std::ostream& operator<<(std::ostream& os, const std::vector<float>& vec){
    os << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        os << vec[i];
        if (i < vec.size() - 1) os << ", ";
    }
    os << "]";
    return os;
}

std::vector<std::tuple<std::string, float, int>> Chunk::ChunkQuery::getRetrieveList() const {
    return m_retrieve_list;
}

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

RAGLibrary::Document Chunk::ChunkQuery::getQuery() const {
    return m_query_doc;
}

std::pair<Chunk::EmbeddingModel, std::string> Chunk::ChunkQuery::getPar() const {
    return { m_embedding_model, m_model };
}

std::vector<RAGLibrary::Document> Chunk::ChunkQuery::getChunksList() const {
    if (m_chunks_list.empty()) {
        throw std::invalid_argument("Empty chunks list.");
    }
    return m_chunks_list;
}

std::vector<std::tuple<std::string, float, int>> Chunk::ChunkQuery::Retrieve(float threshold) {
    if (m_emb_query.empty()) throw std::runtime_error("Query not yet initialized.");
    if (threshold < -1.0f || threshold > 1.0f) throw std::invalid_argument("Threshold out of bound [-1,1].");
    if (m_chunk_embedding.empty() || m_emb_query.empty()) throw std::runtime_error("Embeddings not found.");

    std::vector<std::tuple<std::string, float, int>> scored_hits;
    torch::Tensor()
    auto query_tensor = torch::from_blob(
        const_cast<float*>(m_emb_query.data()),
        {int64_t(m_emb_query.size())}, torch::kFloat32
    );

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

            if (sim >= threshold) {
                local_hits.emplace_back(
                    m_chunks_list[i].page_content,
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

    std::sort(
        scored_hits.begin(),
        scored_hits.end(),
        [](auto &a, auto &b) {
            return std::get<1>(a) > std::get<1>(b);
        }
    );

    m_retrieve_list   = std::move(scored_hits);
    quant_retrieve_list = int(m_retrieve_list.size()); //int quant_retrieve_list = static_cast<int>(m_retrieve_list.size());
    return m_retrieve_list;
}
