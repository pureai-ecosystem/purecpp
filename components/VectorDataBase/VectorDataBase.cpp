#include <VectorDataBase/VectorDataBase.h>

std::optional<VectorDataBase::PureResult>
VectorDataBase::PureL2(std::string query, const Chunk::ChunkDefault& chunks, size_t pos, int k) {
    Chunk::ChunkQuery cq(query, {}, &chunks, pos);
    size_t nq, d, ndb;
    std::tie(nq, d, ndb) = cq.getPar();

    if (k > ndb) {
        throw std::invalid_argument("k > base vector");
    }

    faiss::IndexFlatL2 index(d);

    const Chunk::vdb_data* vdb = cq.getVDB();
    if (!vdb) {
        throw std::runtime_error("vdb_data is null. Cannot proceed.");
    }

    const float* xb = vdb->getVDpointer();
    if (!xb) {
        throw std::runtime_error("Empty vector database. Cannot proceed.");
    }

    index.add(ndb, xb);

    auto emb_query = cq.getEmbedQuery();
    if (emb_query.size() != d) {
        throw std::runtime_error("Embedding dimension mismatch.");
    }

    std::vector<faiss::idx_t> I(k);
    std::vector<float> D(k);
    index.search(nq, emb_query.data(), k, D.data(), I.data());

    if (D.size() > 0) {
        std::cout << "Nearest index: " << I[0] << std::endl;
        std::cout << "Distance: " << D[0] << std::endl;
        return VectorDataBase::PureResult{I, D};
    }
    return {};
}

std::optional<VectorDataBase::PureResult>
VectorDataBase::PureIP(std::string query, const Chunk::ChunkDefault& chunks, size_t pos, int k) {
    Chunk::ChunkQuery cq(query, {}, &chunks, pos);
    size_t nq, d, ndb;
    std::tie(nq, d, ndb) = cq.getPar();

    if (k > ndb) {
        throw std::invalid_argument("k > base vector");
    }

    faiss::IndexFlatIP index(d);

    const Chunk::vdb_data* vdb = cq.getVDB();
    if (!vdb) {
        throw std::runtime_error("vdb_data is null. Cannot proceed.");
    }

    const float* xb = vdb->getVDpointer();
    if (!xb) {
        throw std::runtime_error("Empty vector database. Cannot proceed.");
    }

    index.add(ndb, xb);

    auto emb_query = cq.getEmbedQuery();
    if (emb_query.size() != d) {
        throw std::runtime_error("Embedding dimension mismatch.");
    }

    std::vector<faiss::idx_t> I(k);
    std::vector<float> D(k);
    index.search(nq, emb_query.data(), k, D.data(), I.data());

    if (D.size() > 0) {
        std::cout << "Most similar index: " << I[0] << std::endl;
        std::cout << "Similarity score: " << D[0] << std::endl;
        return VectorDataBase::PureResult{I, D};
    }
    return {};
}

std::optional<VectorDataBase::PureResult>
VectorDataBase::PureCosine(std::string query, const Chunk::ChunkDefault& chunks, size_t pos, int k) {
    Chunk::ChunkQuery cq(query, {}, &chunks, pos);
    size_t nq, d, ndb;
    std::tie(nq, d, ndb) = cq.getPar();

    if (k > ndb) {
        throw std::invalid_argument("k > base vector");
    }

    const Chunk::vdb_data* vdb = cq.getVDB();
    if (!vdb) {
        throw std::runtime_error("vdb_data is null. Cannot proceed.");
    }

    std::vector<float> base = vdb->flatVD;
    for (size_t i = 0; i < ndb; ++i) {
        normalize_vector(&base[i * d], d);
    }

    faiss::IndexFlatIP index(d);
    index.add(ndb, base.data());

    auto emb_query = cq.getEmbedQuery();
    if (emb_query.size() != d) {
        throw std::runtime_error("Embedding dimension mismatch.");
    }

    std::vector<float> normalized_query = emb_query;
    normalize_vector(normalized_query.data(), d);

    std::vector<faiss::idx_t> I(k);
    std::vector<float> D(k);
    index.search(nq, normalized_query.data(), k, D.data(), I.data());

    if (D.size() > 0) {
        return VectorDataBase::PureResult{I, D};
    }
    return {};
}

void VectorDataBase::normalize_vector(float* vec, size_t d) {
    float norm = 0.0f;
    for (size_t i = 0; i < d; ++i) {
        norm += vec[i] * vec[i];
    }
    norm = std::sqrt(norm);
    if (norm > 0.0f) {
        for (size_t i = 0; i < d; ++i) {
            vec[i] /= norm;
        }
    }
}
