cmake_minimum_required(VERSION 3.22)
project(RagPUREAI VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CUDA_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON) 
# set(CMAKE_CXX_EXTENSIONS OFF) 
set(CMAKE_BUILD_TYPE "Release")


# ─────────────────────────────────────────────────────────────────────────────
#------- Caminhos para FAISS -------
# ─────────────────────────────────────────────────────────────────────────────
set(FAISS_ROOT "${CMAKE_BINARY_DIR}/../../../libs/faiss")
set(FAISS_INCLUDE_DIR "${FAISS_ROOT}/faiss")
set(FAISS_LIB_DIR "${FAISS_ROOT}/build/faiss") # Must contain libfaiss.a

add_library(faiss STATIC IMPORTED) # Declares FAISS as an already compiled library (IMPORTED)
set_target_properties(faiss PROPERTIES
    IMPORTED_LOCATION "${FAISS_LIB_DIR}/libfaiss.a"
    INTERFACE_INCLUDE_DIRECTORIES "${FAISS_INCLUDE_DIR}"
)


# # ── FAISS (robusto a layout de build)
# set(FAISS_ROOT "${CMAKE_SOURCE_DIR}/../libs/faiss")
# set(FAISS_INCLUDE_DIR "${FAISS_ROOT}/faiss")
# set(FAISS_LIB_DIR "${FAISS_ROOT}/build/faiss")

# # Aceita tanto .a quanto .so
# find_library(FAISS_LIB NAMES faiss PATHS "${FAISS_LIB_DIR}" NO_DEFAULT_PATH)

# if(NOT FAISS_LIB)
#   message(FATAL_ERROR "FAISS não encontrado em ${FAISS_LIB_DIR}. Construa o FAISS ou ajuste o caminho.")
# endif()

# add_library(faiss UNKNOWN IMPORTED)
# set_target_properties(faiss PROPERTIES
#   IMPORTED_LOCATION "${FAISS_LIB}"
#   INTERFACE_INCLUDE_DIRECTORIES "${FAISS_INCLUDE_DIR}"
# )

# ─────────────────────────────────────────────────────────────────────────────
#------- Compiler Specific flags -------
# ─────────────────────────────────────────────────────────────────────────────
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")# MSVC-specific flags
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_BINARY_DIR}/generators/conan_toolchain.cmake)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Od /EHsc /MP /openmp /MD")
else() # Flags for G++/Clang or G++/GNU on Linux
    # Keeps the optimization flags and adds -std=c++23
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_BINARY_DIR}/Release/generators/conan_toolchain.cmake)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fopenmp -pthread") 
    set(CMAKE_CXX_FLAGS_RELEASE "-O0 -std=c++23") 
endif()

# ─────────────────────────────────────────────────────────────────────────────
#------- Options -------
# ─────────────────────────────────────────────────────────────────────────────
option(CURL_STATIC_LINKING "Set to ON to build libcurl with static linking." OFF)

if(CURL_STATIC_LINKING)
    message("-DCURL_STATICLIB [added]")
    add_definitions(-DCURL_STATICLIB)
endif()


# ─────────────────────────────────────────────────────────────────────────────
#------- Find Python  -------
# ─────────────────────────────────────────────────────────────────────────────
set(Python3_FIND_SHARED OFF)# Need it?
find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(PYTHON_LIB_PATH "C:\\Program Files\\Python312\\libs")
    link_directories(${Python3_LIBRARY_DIRS})
endif()

include_directories(${Python3_INCLUDE_DIRS})

# ─────────────────────────────────────────────────────────────────────────────
#------- Find Pybind11 -------
# ─────────────────────────────────────────────────────────────────────────────
message(STATUS "---------------------------------------\n")
if(Python3_VERSION_MAJOR EQUAL 3 AND Python3_VERSION_MINOR EQUAL 8)
    message(STATUS "Python 3.8 detected!!")
    set(pybind11_DIR "/opt/python/cp38-cp38/lib/python3.8/site-packages/pybind11/share/cmake/pybind11")
    # add_definitions(-DPy_LIMITED_API=0x03080000)

elseif(Python3_VERSION_MAJOR EQUAL 3 AND Python3_VERSION_MINOR EQUAL 9)
    message(STATUS "Python 3.9 detected!!")
    set(pybind11_DIR "/opt/python/cp39-cp39/lib/python3.9/site-packages/pybind11/share/cmake/pybind11")
    # add_definitions(-DPy_LIMITED_API=0x03090000)

elseif(Python3_VERSION_MAJOR EQUAL 3 AND Python3_VERSION_MINOR EQUAL 10)
    message(STATUS "Python 3.10 detected!!")
    set(pybind11_DIR "/opt/python/cp310-cp310/lib/python3.10/site-packages/pybind11/share/cmake/pybind11")
    # add_definitions(-DPy_LIMITED_API=0x030A0000)

elseif(Python3_VERSION_MAJOR EQUAL 3 AND Python3_VERSION_MINOR EQUAL 11)
    message(STATUS "Python 3.11 detected!!")
    set(pybind11_DIR "/opt/python/cp311-cp311/lib/python3.11/site-packages/pybind11/share/cmake/pybind11")
    # add_definitions(-DPy_LIMITED_API=0x030B0000)

elseif(Python3_VERSION_MAJOR EQUAL 3 AND Python3_VERSION_MINOR EQUAL 12)
    message(STATUS "Python 3.12 detected!!")
    set(pybind11_DIR "/opt/python/cp312-cp312/lib/python3.12/site-packages/pybind11/share/cmake/pybind11")
    # add_definitions(-DPy_LIMITED_API=0x030C0000)

elseif(Python3_VERSION_MAJOR EQUAL 3 AND Python3_VERSION_MINOR EQUAL 13)
    message(STATUS "Python 3.13 detected!")
    set(pybind11_DIR "/opt/python/cp313-cp313/lib/python3.13/site-packages/pybind11/share/cmake/pybind11")
    # add_definitions(-DPy_LIMITED_API=0x030D0000)

else()
    message(STATUS "Python version not specified in previous if's!")
    # Default setting or other action
    
endif()


# ─────────────────────────────────────────────────────────────────────────────
#------- Find other dependencies -------
# ─────────────────────────────────────────────────────────────────────────────
find_package(pybind11 REQUIRED)
find_package(pdfium REQUIRED)          # Biblioteca para manipulação de PDFs
find_package(OpenMP REQUIRED)          # OpenMP primeiro, pois pode ser usado por outros pacotes
find_package(ICU REQUIRED)             # Biblioteca de internacionalização
find_package(miniz REQUIRED)           # Biblioteca de compressão
find_package(rapidxml REQUIRED)        # Parser XML
find_package(beauty REQUIRED)          # HTTP Server (geralmente independente)
find_package(lexbor REQUIRED)          # Biblioteca de parsing HTML
find_package(re2 REQUIRED)             # Biblioteca de regex eficiente
find_package(nlohmann_json REQUIRED)   # Biblioteca de JSON (não tem dependências)
#find_package(fmt REQUIRED)            # Biblioteca de formatação de strings em casos aonde n tem format em compiladores mais antigos
find_package(CURL REQUIRED)            # Biblioteca para requisições HTTP
find_package(Threads REQUIRED)         

# ─────────────────────────────────────────────────────────────────────────────
#------- Protobuf -------
# ─────────────────────────────────────────────────────────────────────────────
find_package(Protobuf REQUIRED)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    link_directories(${Protobuf_LIBRARY_DIRS})
    include_directories(C:/vcpkg/packages/protobuf_x64-windows/include)
    if (NOT DEFINED PROTOBUF_PROTOC_EXECUTABLE)
        # include_directories(${Protobuf_INCLUDE_DIRS})# At first the Protobuf_INCLUDE_DIR target does not exist, but I don't know if it is necessary
        set(PROTOBUF_PROTOC_EXECUTABLE "C:/vcpkg/packages/protobuf_x64-windows/tools/protobuf/protoc.exe")
    endif()
endif()

find_package(onnxruntime REQUIRED) # May depend on protobuf (check if you need it!)

# ───────────────────────────────────────────────────────────────────────
#------- Discover the directory Current Python site-packages -------
# ──────────────────────────────────────────────────────────────────────────────
# We will use sysconfig to be more robust across multiple Python versions.
execute_process(
    COMMAND "${Python3_EXECUTABLE}" -c
        "import sysconfig; import pathlib; site_packages = sysconfig.get_paths()['purelib']; print(str(pathlib.Path(site_packages).resolve()))"
    OUTPUT_VARIABLE PYTHON_SITE_PACKAGES
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# ─────────────────────────────────────────────────────────────────────────────
#------- Torch -------
# ─────────────────────────────────────────────────────────────────────────────
# set(_TORCH_REL_PATH "dependencias_libs/d_libs/libtorch/cpu")
set(Torch_DIR "${CMAKE_SOURCE_DIR}/../libs/libtorch/cpu/share/cmake/Torch")

find_package(Torch REQUIRED)
 
# Ajustar includes e bibliotecas do Torch
include_directories("${CMAKE_SOURCE_DIR}/../libs/libtorch/cpu/include")
link_directories("${CMAKE_SOURCE_DIR}/../libs/libtorch/cpu/lib")


# ─────────────────────────────────────────────────────────────────────────────
#------- RPATH -------
# ─────────────────────────────────────────────────────────────────────────────
#------- Configuração do RPATH/RUNPATH/LIBRARY_PATH -------> Ajustar rpath para encontrar as bibliotecas do Torch
set(CMAKE_BUILD_RPATH
    "${PYTHON_SITE_PACKAGES}/*/d_libs/libtorch/cpu/lib"
    "\$ORIGIN/purecpp.libs"
    "\$ORIGIN/d_libs/libtorch/cpu/lib"
    "${CMAKE_SOURCE_DIR}/libs/libtorch/cpu/lib"
    "\$ORIGIN/libs/libtorch/cpu/lib"
    "\$ORIGIN:/usr/bin/protoc"
    "/usr/lib/x86_64-linux-gnu"
    "/usr/lib64"
    "\$ORIGIN/"
    "\$ORIGIN/purecpp/d_libs/libtorch/cpu/lib"
)

set(CMAKE_INSTALL_RPATH
    "${PYTHON_SITE_PACKAGES}/*/d_libs/libtorch/cpu/lib"
    "\$ORIGIN/purecpp.libs"
    "\$ORIGIN/d_libs/libtorch/cpu/lib"
    "${CMAKE_SOURCE_DIR}/libs/libtorch/cpu/lib"
    "\$ORIGIN/libs/libtorch/cpu/lib"
    "\$ORIGIN:/usr/bin/protoc"
    "/usr/lib/x86_64-linux-gnu"
    "/usr/lib64"
    "\$ORIGIN/"
    "\$ORIGIN/purecpp/d_libs/libtorch/cpu/lib"
)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--enable-new-dtags")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)



# ─────────────────────────────────────────────────────────────────────────────
#------- Tokenizers -------
# ─────────────────────────────────────────────────────────────────────────────
set(TOKENIZERS_PATH "${CMAKE_SOURCE_DIR}/../libs/tokenizers-cpp")
add_subdirectory(${TOKENIZERS_PATH} tokenizers EXCLUDE_FROM_ALL)

# ─────────────────────────────────────────────────────────────────────────────
#------- OpenAI C++ bindings -------
# ─────────────────────────────────────────────────────────────────────────────
set(OPENAI_CPP_PATH "${CMAKE_SOURCE_DIR}/../libs/openai-cpp")


# ─────────────────────────────────────────────────────────────────────────────
# ----- Project sources -----
# ─────────────────────────────────────────────────────────────────────────────
set(RagPUREAI_BINDING_SRCS  "${CMAKE_SOURCE_DIR}/binding.cpp") 

set(RagPUREAI_IMPL_SRCS
    ${CMAKE_SOURCE_DIR}/../libs/StringUtils/StringUtils.cpp

    ${CMAKE_SOURCE_DIR}/../components/Embedding/BaseEmbedding.cpp
    ${CMAKE_SOURCE_DIR}/../components/Embedding/EmbeddingOpenAI/EmbeddingOpenAI.cpp

    ${CMAKE_SOURCE_DIR}/../components/Chunk/ChunkCommons/ChunkCommons.cpp
    ${CMAKE_SOURCE_DIR}/../components/Chunk/ChunkCount/ChunkCount.cpp
    ${CMAKE_SOURCE_DIR}/../components/Chunk/ChunkDefault/ChunkDefault.cpp
    ${CMAKE_SOURCE_DIR}/../components/Chunk/ChunkSimilarity/ChunkSimilarity.cpp
    ${CMAKE_SOURCE_DIR}/../components/Chunk/ChunkQuery/ChunkQuery.cpp
    
    ${CMAKE_SOURCE_DIR}/../components/CleanData/ContentCleaner/ContentCleaner.cpp
    ${CMAKE_SOURCE_DIR}/../components/FAISSVectorSearch/FAISSVectorSearch.cpp
)


# ─────────────────────────────────────────────────────────────────────────────
#------- RagPUREAI Library -------
# ─────────────────────────────────────────────────────────────────────────────

add_library(RagPUREAILib STATIC ${RagPUREAI_IMPL_SRCS})

target_include_directories(RagPUREAILib PUBLIC #.h
    ${CMAKE_SOURCE_DIR}/../libs/RagException
    ${CMAKE_SOURCE_DIR}/../libs/ThreadSafeQueue
    ${CMAKE_SOURCE_DIR}/../libs/CommonStructs
    ${CMAKE_SOURCE_DIR}/../libs/StringUtils
    ${CMAKE_SOURCE_DIR}/../libs/FileUtils
    ${CMAKE_SOURCE_DIR}/../libs/MemoryUtils
    #C:/vcpkg/packages/protobuf_x64-windows/include
    ${CURL_INCLUDE_DIRS}
    ${OPENAI_CPP_PATH}/include
    ${TORCH_INCLUDE_DIRS}
    ${TOKENIZERS_PATH}/include
    ${FAISS_INCLUDE_DIR}

    ${CMAKE_SOURCE_DIR}/../components
    ${CMAKE_SOURCE_DIR}/../components/DataLoader
    ${CMAKE_SOURCE_DIR}/../components/MetadataExtractor
    ${CMAKE_SOURCE_DIR}/../components/Chunk
    ${CMAKE_SOURCE_DIR}/../components/Chunk/ChunkCommons
    ${CMAKE_SOURCE_DIR}/../components/CleanData/ContentCleaner
    ${CMAKE_SOURCE_DIR}/../components/Embedding/EmbeddingOpenAI
    ${CMAKE_SOURCE_DIR}/../components/Embedding
    ${CMAKE_SOURCE_DIR}/../components/FAISSVectorSearch
    ${CMAKE_SOURCE_DIR}/../libs/faiss/
)

link_directories(${FAISS_LIB_DIR})

# ─────────────────────────────────────────────────────────────────────────────
# Link libraries with RagPUREAILib
# ─────────────────────────────────────────────────────────────────────────────
target_link_libraries(RagPUREAILib PUBLIC
    pdfium::pdfium
    icu::icu
    miniz::miniz
    rapidxml::rapidxml
    beauty::beauty
    #fmt::fmt
    lexbor::lexbor_static       
    re2::re2
    protobuf::libprotobuf      
    onnxruntime::onnxruntime    
    OpenMP::OpenMP_CXX         
    Threads::Threads 
    CURL::libcurl
    nlohmann_json::nlohmann_json
    ${Python3_LIBRARIES}
    ${TORCH_LIBRARIES}        
    tokenizers_cpp
    faiss
)
# ─────────────────────────────────────────────────────────────────────────────
#---------------- Pybind11 Module
# ─────────────────────────────────────────────────────────────────────────────

pybind11_add_module(RagPUREAI ${RagPUREAI_BINDING_SRCS})

target_link_libraries(RagPUREAI PRIVATE RagPUREAILib)


# ─────────────────────────────────────────────────────────────────────────────
#---------------- DEBUG MESSAGES
# ─────────────────────────────────────────────────────────────────────────────
#${CMAKE_SOURCE_DIR})