#include <faiss/IndexFlat.h>
#include <iostream>
#include <vector>

int main() {
    // Dimensão dos vetores
    int d = 4;

    // Criação de um índice FAISS de distância L2
    faiss::IndexFlatL2 index(d);

    // Vetores para adicionar ao índice
    std::vector<float> xb = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    };

    int nb = 4; // número de vetores

    // Adiciona os vetores ao índice
    index.add(nb, xb.data());
    //---------------------------------------------------------------
    // Vetor de consulta
    std::vector<float> xq = {0.9, 0.1, 0.0, 0.0};
    int nq = 1; // número de queries

    // Resultados
    std::vector<faiss::idx_t> I(nq); // índices
    std::vector<float> D(nq); // distâncias

    // Busca do vetor mais próximo
    index.search(nq, xq.data(), 1, D.data(), I.data());

    // Impressão do resultado
    std::cout << "Índice mais próximo: " << I[0] << std::endl;
    std::cout << "Distância: " << D[0] << std::endl;

    return 0;
}
