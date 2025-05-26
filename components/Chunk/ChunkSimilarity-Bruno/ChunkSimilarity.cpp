#include "ChunkCommons/ChunkCommons.h"
#include "ChunkSimilarity.h"
#include "RagException.h"
#include "StringUtils.h"

#include <cmath>
#include <fstream>
#include <omp.h>
#include <syncstream>
#include <torch/torch.h>

using namespace Chunk;

ChunkSimilarity::ChunkSimilarity(
    const int chunk_size,
    const int overlap,
    Chunk::EmbeddingModel embedding_model,
    std::string model
)
    : m_chunk_size(chunk_size),
      m_overlap(overlap),
      m_embedding_model(embedding_model), 
      m_model(std::move(model))
{
    ValidateModel();
}

//========================== APAGAR ==========================
void ChunkSimilarity::ValidateModel()
{
    if (m_overlap >= m_chunk_size)
    {
        throw RAGLibrary::RagException("The overlap value must be smaller than the chunk size.");
    }

    switch (m_embedding_model)
    {
    case HuggingFace:
        break;
    case OpenAI:
        const auto open_api_key = std::getenv("OPENAI_API_KEY");
        if (m_openai_api_key.empty() && (!open_api_key || std::strcmp("", open_api_key) == 0))
        {
            throw RAGLibrary::RagException("The OpenAI API key is required to use the 'openai' model.");
        }
        break;
    }
}
//========================== APAGAR ==========================

//========================== APAGAR ==========================
std::vector<std::vector<float>> ChunkSimilarity::GenerateEmbeddings(const std::vector<std::string> &chunks)
{
    std::vector<std::vector<float>> results;
    switch (this->m_embedding_model)
    {
    case EmbeddingModel::HuggingFace:
        results = Chunk::EmbeddingHuggingFaceTransformers(chunks);
        break;
    case EmbeddingModel::OpenAI:
        results = Chunk::EmbeddingOpeanAI(chunks, m_openai_api_key);
        break;
    }

    return results;
}
//========================== APAGAR ==========================

std::vector<RAGLibrary::Document> ChunkSimilarity::Reorder()
{
    if (m_chunks_list.empty()) { throw std::invalid_argument("Empty chunks list."); }

    this->m_chunk_embedding.resize(this->m_chunks_list.size());

    if (this->allChunksHaveEmbeddings(this->m_chunks_list)) {
        for (size_t i = 0; i < this->m_chunks_list.size(); ++i) {
            this->m_chunk_embedding[i] = this->m_chunks_list[i].embedding.value();
        }
    } else {
        auto docs = this->Embeddings(m_chunks_list);
        this->m_chunks_list = std::move(docs);

        for (size_t i = 0; i < this->m_chunks_list.size(); ++i) {
            if (!this->m_chunks_list[i].embedding.has_value()) {
                throw std::runtime_error("Missing embedding after Embeddings() call");
            }
            this->m_chunk_embedding[i] = this->m_chunks_list[i].embedding.value();
        }
    }

    std::vector<RAGLibrary::Document> documents;

    try {
        auto embeddingsTensor = Chunk::toTensor(this->m_chunk_embedding);

        auto similarity_matrix = torch::inner(embeddingsTensor, embeddingsTensor);
        auto sorted_indices = torch::argsort(-similarity_matrix.sum(1));

        documents.reserve(m_chunks_list.size());

        for (int i = 0; i < sorted_indices.size(0); ++i) {
            int64_t idx = sorted_indices[i].item<int64_t>();
            documents.push_back(m_chunks_list[idx]);  // REORDENA diretamente
        }
    } catch (const std::exception &e) {
        std::cerr << "[Reorder] " << e.what() << std::endl;
        throw;
    }

    this->m_chunks_list = std::move(documents);
    return this->m_chunks_list;
}

std::vector<RAGLibrary::Document> ChunkSimilarity::ProcessDocuments(const std::vector<RAGLibrary::Document> &items, int max_workers)
{
    // Validation of input parameters ----------------------------------------
    if (items.empty()) { throw std::invalid_argument("Empty items list."); }
    if (max_workers < 1) { max_workers = 4; }
    if (max_workers > 8) { max_workers = 8; }
    //------------------------------------------------------------------------

    std::vector<RAGLibrary::Document> documents;  // <- fora da região paralela

    try
    {
        int max_threads = omp_get_max_threads();
        if (max_workers < max_threads)
        {
            max_threads = max_workers;
        }

        omp_set_num_threads(max_threads);

        #pragma omp parallel
        {
            std::vector<RAGLibrary::Document> local_docs;

            #pragma omp for nowait
            for (int i = 0; i < items.size(); i++)
            {
                const auto& item = items[i];
                auto chunks = Chunk::SplitText(item.page_content, m_overlap, m_chunk_size);
                for (auto &chunk : chunks)
                {
                    local_docs.emplace_back(item.metadata, chunk);
                }
            }

            #pragma omp critical
            {
                documents.insert(documents.end(),
                                 std::make_move_iterator(local_docs.begin()),
                                 std::make_move_iterator(local_docs.end()));
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        throw;
    }

    this->m_chunks_list = std::move(documents);  // <- agora está correto

    return this->m_chunks_list;
}


//-------------------------- Funcoes Bruno --------------------------------------------
std::vector<RAGLibrary::Document> ChunkSimilarity::getChunksList() const {
    if (this->m_chunks_list.empty()) {
        throw std::invalid_argument("Empty chunks list.");
    }
    return this->m_chunks_list;
}

std::vector<RAGLibrary::Document> 
ChunkSimilarity::Similarity(const std::vector<RAGLibrary::Document> &items, int max_workers)
{
    // Validation of input parameters ----------------------------------------
    if (items.empty()) { throw std::invalid_argument("Empty items list."); }
    if (max_workers <  1) { max_workers = 4; }
    if (max_workers >  8) { max_workers = 8; }// Can be ajusted in the near future
    //------------------------------------------------------------------------

    this->ProcessDocuments(items, max_workers);

    return this->Reorder();   
}


void ChunkSimilarity::InitAPIKey() {
    const char* env_key = std::getenv("OPENAI_API_KEY");
    this->m_openai_api_key = (env_key != nullptr) ? std::string(env_key) : "";
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

inline bool 
ChunkSimilarity::allChunksHaveEmbeddings(const std::vector<RAGLibrary::Document>& chunks_list) {
    return std::all_of(
        chunks_list.begin(),
        chunks_list.end(),
        [](const RAGLibrary::Document& doc) {
            return doc.embedding.has_value();
        });
}

std::vector<RAGLibrary::Document> 
ChunkSimilarity::Embeddings(const std::vector<RAGLibrary::Document>& list)
{
    if (this->m_embedding_model !=  Chunk::EmbeddingModel::OpenAI){
        throw std::invalid_argument("Model not yet supported");
    }
    ChunkSimilarity::InitAPIKey();
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
                emb = OpenAI->GenerateEmbeddings(list, m_model);//verificar se ta retornando certo
                count++;
            }while(!ChunkSimilarity::allChunksHaveEmbeddings(emb) && count < 3);

            if (!ChunkSimilarity::allChunksHaveEmbeddings(emb)) {
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