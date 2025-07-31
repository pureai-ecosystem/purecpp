cmake_minimum_required(VERSION 3.22)
project(RagPUREAI VERSION 1.0)

# General build settings
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Toolchain
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_BINARY_DIR}/generators/conan_toolchain.cmake)
    set(CMAKE_CXX_FLAGS_RELEASE "/Od")
else()
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_BINARY_DIR}/Release/generators/conan_toolchain.cmake)
    set(CMAKE_CXX_FLAGS_RELEASE "-O0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()

option(CURL_STATIC_LINKING "Set to ON to build libcurl with static linking." OFF)
option(BUILD_APPS "Build apps" OFF)

# Python & Pybind11
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
include_directories(${Python3_INCLUDE_DIRS})
find_package(pybind11 REQUIRED)

# External Dependencies
find_package(pdfium REQUIRED)
find_package(ICU REQUIRED)
find_package(miniz REQUIRED)
find_package(rapidxml REQUIRED)
find_package(beauty REQUIRED)
find_package(lexbor REQUIRED)
find_package(OpenMP REQUIRED)
find_package(re2 REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(CURL REQUIRED)
find_package(onnxruntime REQUIRED)
find_package(redis++ REQUIRED)

execute_process(
    COMMAND "${Python3_EXECUTABLE}" -c
        "import sysconfig; import pathlib; site_packages = sysconfig.get_paths()['purelib']; print(str(pathlib.Path(site_packages).resolve()))"
    OUTPUT_VARIABLE PYTHON_SITE_PACKAGES
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Protobuf
find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})

# Torch
set(Torch_DIR "${CMAKE_SOURCE_DIR}/libs/libtorch/cpu/share/cmake/Torch")
find_package(Torch REQUIRED)
include_directories("${CMAKE_SOURCE_DIR}/libs/libtorch/cpu/include")
link_directories("${CMAKE_SOURCE_DIR}/libs/libtorch/cpu/lib")

# Tokenizers
set(TOKENIZERS_PATH "${CMAKE_SOURCE_DIR}/libs/tokenizers-cpp")
add_subdirectory(${TOKENIZERS_PATH} tokenizers EXCLUDE_FROM_ALL)

# OpenAI
set(OPEANAI_CPP_PATH "${CMAKE_SOURCE_DIR}/libs/openai-cpp")

# RPATH
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

# Sources
file(GLOB_RECURSE VDB_SRCS ${CMAKE_SOURCE_DIR}/components/VectorDatabase/src/*.cpp)

set(RagPUREAI_BINDING_SRCS
    ${CMAKE_SOURCE_DIR}/src/binding.cpp
    ${CMAKE_SOURCE_DIR}/components/VectorDatabase/python/binding_vectordb.cpp
)

set(RagPUREAI_IMPL_SRCS
    ${VDB_SRCS}
    ${CMAKE_SOURCE_DIR}/libs/StringUtils/StringUtils.cpp
    ${CMAKE_SOURCE_DIR}/components/DataLoader/BaseLoader.cpp
    ${CMAKE_SOURCE_DIR}/components/DataLoader/PDFLoader/PDFLoader.cpp
    ${CMAKE_SOURCE_DIR}/components/DataLoader/DOCXLoader/DOCXLoader.cpp
    ${CMAKE_SOURCE_DIR}/components/DataLoader/WebLoader/WebLoader.cpp
    ${CMAKE_SOURCE_DIR}/components/DataLoader/TXTLoader/TXTLoader.cpp

    ${CMAKE_SOURCE_DIR}/components/MetadataExtractor/MetadataExtractor.cpp
    ${CMAKE_SOURCE_DIR}/components/MetadataExtractor/MetadataRegexExtractor/MetadataRegexExtractor.cpp
    ${CMAKE_SOURCE_DIR}/components/MetadataExtractor/MetadataHFExtractor/MetadataHFExtractor.cpp

    ${CMAKE_SOURCE_DIR}/components/Embedding/EmbeddingOpenAI/EmbeddingOpenAI.cpp
    ${CMAKE_SOURCE_DIR}/components/Embedding/EmbeddingModel/EmbeddingModel.cpp

    ${CMAKE_SOURCE_DIR}/components/Chunk/ChunkCommons/ChunkCommons.cpp
    ${CMAKE_SOURCE_DIR}/components/Chunk/ChunkCount/ChunkCount.cpp
    ${CMAKE_SOURCE_DIR}/components/Chunk/ChunkDefault/ChunkDefault.cpp
    ${CMAKE_SOURCE_DIR}/components/Chunk/ChunkSimilarity/ChunkSimilarity.cpp
    ${CMAKE_SOURCE_DIR}/components/Chunk/ChunkQuery/ChunkQuery.cpp

    ${CMAKE_SOURCE_DIR}/components/CleanData/ContentCleaner/ContentCleaner.cpp
)

# RagPUREAILib
add_library(RagPUREAILib STATIC ${RagPUREAI_IMPL_SRCS})
target_include_directories(RagPUREAILib PUBLIC
    ${CMAKE_SOURCE_DIR}/components
    ${CMAKE_SOURCE_DIR}/components/DataLoader
    ${CMAKE_SOURCE_DIR}/components/MetadataExtractor
    ${CMAKE_SOURCE_DIR}/components/Chunk
    ${CMAKE_SOURCE_DIR}/components/CleanData
    ${CMAKE_SOURCE_DIR}/components/Embedding
    ${CMAKE_SOURCE_DIR}/components/Embedding/EmbeddingOpenAI
    ${CMAKE_SOURCE_DIR}/components/Embedding/EmbeddingModel
    ${CMAKE_SOURCE_DIR}/components/VectorDatabase/include
    ${CMAKE_SOURCE_DIR}/libs/RagException
    ${CMAKE_SOURCE_DIR}/libs/ThreadSafeQueue
    ${CMAKE_SOURCE_DIR}/libs/CommonStructs
    ${CMAKE_SOURCE_DIR}/libs/StringUtils
    ${CMAKE_SOURCE_DIR}/libs/FileUtils
    ${CMAKE_SOURCE_DIR}/libs/MemoryUtils
    ${TOKENIZERS_PATH}/include
    ${OPEANAI_CPP_PATH}/include
    ${CMAKE_SOURCE_DIR}/libs/libtorch/cpu/include
    ${CURL_INCLUDE_DIRS}
    ${TORCH_INCLUDE_DIRS}
)

target_link_libraries(RagPUREAILib PUBLIC
    pdfium::pdfium
    icu::icu
    miniz::miniz
    rapidxml::rapidxml
    beauty::beauty
    lexbor::lexbor_static
    OpenMP::OpenMP_CXX
    re2::re2
    nlohmann_json::nlohmann_json
    redis++::redis++_static
    hiredis::hiredis
    onnxruntime::onnxruntime
    tokenizers_cpp
    protobuf::libprotobuf
    CURL::libcurl
    ${Python3_LIBRARIES}
    ${TORCH_LIBRARIES}
)

# Binding with Pybind11
pybind11_add_module(RagPUREAI ${RagPUREAI_BINDING_SRCS})
target_link_libraries(RagPUREAI PRIVATE RagPUREAILib)
