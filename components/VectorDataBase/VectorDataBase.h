#pragma once

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory> 
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <syncstream>
#include <omp.h>
#include <torch/torch.h>
#include <faiss/IndexFlat.h>

#include "RagException.h"
#include "StringUtils.h"
#include "CommonStructs.h"
#include "EmbeddingOpenAI.h"
#include "Chunk/ChunkCommons/ChunkCommons.h"
#include "Chunk/ChunkDefault/ChunkDefault.h"
#include "Chunk/ChunkQuery/ChunkQuery.h"

namespace VectorDataBase {

    // struct vdb_data {
    //     std::vector<float> flatVD;
    //     std::string vendor;
    //     std::string model;
    //     size_t dim = 0;
    //     size_t n = 0;

    //     inline const std::tuple<size_t, size_t> getPar(void) const {
    //         return {n, dim};
    //     }

    //     inline std::pair<std::string, std::string> getEmbPar(void) const {
    //         return {vendor, model};
    //     }

    //     inline const float* getVDpointer(void) const {
    //         if (flatVD.empty()) {
    //             std::cout << "[Info] Empty Vector Data Base\n";
    //             return {};
    //         }
    //         return flatVD.data();
    //     }
    // };

    struct PureResult {
        std::vector<faiss::idx_t> indices;
        std::vector<float> distances;
    };

    // L2 distance (Euclidean)
    std::optional<PureResult>
    PureL2(std::string query, const Chunk::ChunkDefault& chunks, size_t pos, int k);

    // Inner Product (dot product)
    std::optional<PureResult>
    PureIP(std::string query, const Chunk::ChunkDefault& chunks, size_t pos, int k);

    // Cosine similarity (requires normalization)
    std::optional<PureResult>
    PureCosine(std::string query, const Chunk::ChunkDefault& chunks, size_t pos, int k);

    // Utility: in-place L2 normalization
    void normalize_vector(float* vec, size_t d);

}
