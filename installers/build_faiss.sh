cd libs/Faiss/
cmake -B build -DFAISS_ENABLE_PYTHON=OFF -DFAISS_ENABLE_GPU=OFF
cmake --build build -j$(nproc)