cmake_minimum_required(VERSION 3.22)
project(RagPUREAI VERSION 1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    #set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /fPIC /EHsc /Ox /DNDEBUG")
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest")
    #set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Od")
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_BINARY_DIR}/generators/conan_toolchain.cmake)
    set(CMAKE_CXX_FLAGS_RELEASE "/Od")
else()
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_BINARY_DIR}/Release/generators/conan_toolchain.cmake)
    set(CMAKE_CXX_FLAGS_RELEASE "-O0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()

option(CURL_STATIC_LINKING "Set to ON to build libcurl with static linking." OFF)
option(BUILD_APPS "Build apps" OFF)

if(CURL_STATIC_LINKING)
    message("-DCURL_STATICLIB [added]")
    add_definitions(-DCURL_STATICLIB)
endif()

# Find Python and Pybind11
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(PYTHON_LIB_PATH "C:\\Program Files\\Python312\\libs")
    link_directories(${Python3_LIBRARY_DIRS})
endif()

include_directories(${Python3_INCLUDE_DIRS})
find_package(pybind11 REQUIRED)

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

#------- Protobuf -------
find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    link_directories(${Protobuf_LIBRARY_DIRS})
    include_directories(C:/vcpkg/packages/protobuf_x64-windows/include)
    if (NOT DEFINED PROTOBUF_PROTOC_EXECUTABLE)
        set(PROTOBUF_PROTOC_EXECUTABLE "C:/vcpkg/packages/protobuf_x64-windows/tools/protobuf/protoc.exe")
    endif()
endif()

#------- Torch -------
set(Torch_DIR "${CMAKE_SOURCE_DIR}/libs/libtorch/cpu/share/cmake/Torch")
find_package(Torch REQUIRED)
include_directories("${CMAKE_SOURCE_DIR}/libs/libtorch/cpu/include") #.h
link_directories("${CMAKE_SOURCE_DIR}/libs/libtorch/cpu/lib") #.lib e .dir

#------- Tokenizers -------
set(TOKENIZERS_PATH "${CMAKE_SOURCE_DIR}/libs/tokenizers-cpp")
add_subdirectory(${TOKENIZERS_PATH} tokenizers EXCLUDE_FROM_ALL)

#------- OpenAI C++ bindings -------
set(OPEANAI_CPP_PATH "${CMAKE_SOURCE_DIR}/libs/openai-cpp")

# ----- project -----
#set(RagPUREAI_MAIN ${CMAKE_SOURCE_DIR}/main.cpp)

set(RagPUREAI_BINDING_SRCS  ${CMAKE_SOURCE_DIR}/src/binding.cpp)
set(RagPUREAI_IMPL_SRCS
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

#------- RagPUREAI Library -------
add_library(RagPUREAILib STATIC ${RagPUREAI_IMPL_SRCS})
# add_library(RagPUREAILib ${RagPUREAI_IMPL_SRCS})

target_include_directories(RagPUREAILib PUBLIC #.h
    ${CMAKE_SOURCE_DIR}/components
    ${CMAKE_SOURCE_DIR}/components/DataLoader
    ${CMAKE_SOURCE_DIR}/components/MetadataExtractor
    ${CMAKE_SOURCE_DIR}/components/Chunk
    ${CMAKE_SOURCE_DIR}/components/CleanData
    ${CMAKE_SOURCE_DIR}/components/Embedding
    ${CMAKE_SOURCE_DIR}/components/Embedding/EmbeddingOpenAI
    ${CMAKE_SOURCE_DIR}/components/Embedding/EmbeddingModel
    ${CMAKE_SOURCE_DIR}/libs/RagException
    ${CMAKE_SOURCE_DIR}/libs/ThreadSafeQueue
    ${CMAKE_SOURCE_DIR}/libs/CommonStructs
    ${CMAKE_SOURCE_DIR}/libs/StringUtils
    ${CMAKE_SOURCE_DIR}/libs/FileUtils
    ${TOKENIZERS_PATH}/include
    ${OPEANAI_CPP_PATH}/include
    ${CMAKE_SOURCE_DIR}/libs/libtorch/cpu/include
    ${TOKENIZERS_PATH}/include
    ${CURL_INCLUDE_DIRS}
    #${Python3_INCLUDE_DIRS}
    #${Protobuf_INCLUDE_DIRS}
    #${Python3_LIBRARY_DIRS}
    #${Protobuf_LIBRARY_DIRS}
    # C:/vcpkg/packages/protobuf_x64-windows/include
)

target_link_libraries(RagPUREAILib PUBLIC #.lib e .dll
    pdfium::pdfium
    icu::icu
    miniz::miniz
    rapidxml::rapidxml
    beauty::beauty
    lexbor::lexbor_static
    OpenMP::OpenMP_CXX
    re2::re2
    nlohmann_json::nlohmann_json
    onnxruntime::onnxruntime
    tokenizers_cpp
    protobuf::libprotobuf
    CURL::libcurl
    #${CURL_LIBRARIES}
    ${Python3_LIBRARIES}
    ${TORCH_LIBRARIES}
    #Python3::Python
)
#message(STATUS "\n\n\n\n\n\n\n\n\n\nTORCH_LIBRARIES: ${TORCH_LIBRARIES}\n\n\n\n\n\n\n\n\n\n")

# Pybind11 Module
#pybind11_add_module(RagPUREAI ${RagPUREAI_IMPL_SRCS})
pybind11_add_module(RagPUREAI ${RagPUREAI_BINDING_SRCS})

target_link_libraries(RagPUREAI PRIVATE RagPUREAILib)
# if(BUILD_APPS)
#     message(STATUS "Building apps")
#     add_subdirectory(apps)
# endif()
