cmake_minimum_required(VERSION 3.10)
project(main)

# Define padrão do C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Caminhos para FAISS
set(FAISS_ROOT "/home/brunobzaffari/RagPUREAI/libs/faiss/faiss")
set(FAISS_INCLUDE_DIR "${FAISS_ROOT}")
set(FAISS_LIB_DIR "${FAISS_ROOT}/build/faiss") # Deve conter libfaiss.a

# Headers da FAISS
include_directories(${FAISS_INCLUDE_DIR})

# Biblioteca da FAISS
link_directories(${FAISS_LIB_DIR})

# OpenMP
find_package(OpenMP REQUIRED)

# Executável principal
add_executable(main main.cpp)

# Linka com a FAISS estática e OpenMP
target_link_libraries(main PRIVATE faiss OpenMP::OpenMP_CXX)


# Se for usar FAISS dinâmica (libfaiss.so), troque por:
# target_link_libraries(main PRIVATE faiss_shared)

