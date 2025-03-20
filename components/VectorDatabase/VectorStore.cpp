#include "VectorStore.h"

std::string generate_uuid()
{
    uuid_t uuid;
    char uuid_str[37];
    uuid_generate_random(uuid);
    uuid_unparse(uuid, uuid_str);
    return std::string(uuid_str);
}

VectorStore::VectorStore(int dim) : index(dim), expected_dimensionality(dim) {}

std::vector<float> VectorStore::adjust_embedding_dimension(const std::vector<float> &embedding)
{
    std::vector<float> adjusted_embedding = embedding;
    if (embedding.size() < expected_dimensionality)
    {
        adjusted_embedding.resize(expected_dimensionality, 0.0f);
    }
    else if (embedding.size() > expected_dimensionality)
    {
        adjusted_embedding.resize(expected_dimensionality);
    }
    return adjusted_embedding;
}

void VectorStore::insert_documents(const std::vector<std::pair<LoaderDataStruct, std::vector<float>>> &documents)
{
    std::vector<std::future<void>> futures;
    for (const auto &doc : documents)
    {
        futures.push_back(std::async(std::launch::async, &VectorStore::insert_document, this, doc.first, doc.second));
    }
    for (auto &f : futures)
    {
        f.get();
    }
}

void VectorStore::insert_document(const LoaderDataStruct &document, const std::vector<float> &embedding)
{
    std::string id = generate_uuid();
    std::vector<float> adjusted_embedding = adjust_embedding_dimension(embedding);
    document_store[id] = document;
    embeddings.push_back(adjusted_embedding);
    index.add(1, adjusted_embedding.data());
}

std::vector<json> VectorStore::query_by_embedding(const std::vector<float> &embedding, int top_k)
{
    std::vector<float> adjusted_embedding = adjust_embedding_dimension(embedding);
    std::vector<float> distances(top_k);
    std::vector<faiss::idx_t> indices(top_k);

    index.search(1, adjusted_embedding.data(), top_k, distances.data(), indices.data());

    std::vector<json> results;
    for (int i = 0; i < top_k; i++)
    {
        if (indices[i] < 0 || static_cast<size_t>(indices[i]) >= embeddings.size())
            continue;

        auto it = document_store.begin();
        std::advance(it, indices[i]);
        results.push_back({{"metadata", it->second.metadata.data}, {"textContent", it->second.textContent}, {"score", distances[i]}});
    }
    return results;
}

std::vector<json> VectorStore::hybrid_query(const std::string &query_text, const std::vector<float> &embedding, int top_k)
{
    std::vector<float> adjusted_embedding = adjust_embedding_dimension(embedding);
    std::vector<float> distances(top_k);
    std::vector<faiss::idx_t> indices(top_k);

    index.search(1, adjusted_embedding.data(), top_k, distances.data(), indices.data());

    std::vector<json> results;
    for (int i = 0; i < top_k; i++)
    {
        if (indices[i] < 0 || static_cast<size_t>(indices[i]) >= embeddings.size())
            continue;

        auto it = document_store.begin();
        std::advance(it, indices[i]);
        results.push_back({{"metadata", it->second.metadata.data}, {"textContent", it->second.textContent}, {"score", distances[i]}});
    }
    return results;
}
