#ifndef VECTOR_STORE_H
#define VECTOR_STORE_H

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <random>
#include <future>
#include <uuid/uuid.h>
#include <faiss/IndexFlat.h>
#include <nlohmann/json.hpp>
#include "CommonStructs.h"

using json = nlohmann::json;
using RAGLibrary::LoaderDataStruct;

class VectorStore
{
private:
    faiss::IndexFlatL2 index;
    std::unordered_map<std::string, LoaderDataStruct> document_store;
    std::vector<std::vector<float>> embeddings;
    int expected_dimensionality;

    std::vector<float> adjust_embedding_dimension(const std::vector<float> &embedding);

public:
    VectorStore(int dim = 384);

    void insert_documents(const std::vector<std::pair<LoaderDataStruct, std::vector<float>>> &documents);
    void insert_document(const LoaderDataStruct &document, const std::vector<float> &embedding);
    std::vector<json> query_by_embedding(const std::vector<float> &embedding, int top_k = 5);
    std::vector<json> hybrid_query(const std::string &query_text, const std::vector<float> &embedding = {}, int top_k = 5);
};

std::string generate_uuid();

#endif // VECTOR_STORE_H
