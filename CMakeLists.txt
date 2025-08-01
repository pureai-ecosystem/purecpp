cmake_minimum_required(VERSION 3.22)
project(RagPUREAI VERSION 1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

execute_process(
    COMMAND "${Python3_EXECUTABLE}" -c
        "import sysconfig; import pathlib; site_packages = sysconfig.get_paths()['purelib']; print(str(pathlib.Path(site_packages).resolve()))"
    OUTPUT_VARIABLE PYTHON_SITE_PACKAGES
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

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
include_directories("${CMAKE_SOURCE_DIR}/libs/libtorch/cpu/include")
link_directories("${CMAKE_SOURCE_DIR}/libs/libtorch/cpu/lib")

# -------------------------------------------------------
# === VectorDatabase (lib C++) ==========================
# -------------------------------------------------------
find_package(redis++ REQUIRED)   # hiredis vem junto

file(GLOB_RECURSE VDB_SRCS  ${CMAKE_SOURCE_DIR}/components/VectorDatabase/src/*.cpp)
file(GLOB_RECURSE VDB_INCS  ${CMAKE_SOURCE_DIR}/components/VectorDatabase/include/vectordb/*.h)

add_library(VectorDatabase STATIC ${VDB_SRCS} ${VDB_INCS})
target_include_directories(VectorDatabase PUBLIC
    ${CMAKE_SOURCE_DIR}/components/VectorDatabase/include
)
target_link_libraries(VectorDatabase
    PUBLIC
        nlohmann_json::nlohmann_json
        redis++::redis++_static
        hiredis::hiredis
)

# ------------------------------------------------------------------

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

#------- Tokenizers -------
set(TOKENIZERS_PATH "${CMAKE_SOURCE_DIR}/libs/tokenizers-cpp")
add_subdirectory(${TOKENIZERS_PATH} tokenizers EXCLUDE_FROM_ALL)

#------- OpenAI C++ bindings -------
set(OPEANAI_CPP_PATH "${CMAKE_SOURCE_DIR}/libs/openai-cpp")

# ----- project -----
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

    ${CMAKE_SOURCE_DIR}/components/Chat/Message/HumanMessage.cpp
    ${CMAKE_SOURCE_DIR}/components/Chat/Message/AIMessage.cpp
    ${CMAKE_SOURCE_DIR}/components/Chat/Message/SystemMessage.cpp
    ${CMAKE_SOURCE_DIR}/components/Chat/ChatHistory/ChatHistory.cpp

)

#------- RagPUREAI Library -------
add_library(RagPUREAILib STATIC ${RagPUREAI_IMPL_SRCS})

target_include_directories(RagPUREAILib PUBLIC #.h
    ${CMAKE_SOURCE_DIR}/components
    ${CMAKE_SOURCE_DIR}/components/DataLoader
    ${CMAKE_SOURCE_DIR}/components/MetadataExtractor
    ${CMAKE_SOURCE_DIR}/components/Chunk
    ${CMAKE_SOURCE_DIR}/components/CleanData
    ${CMAKE_SOURCE_DIR}/components/Embedding
    ${CMAKE_SOURCE_DIR}/components/Embedding/EmbeddingOpenAI
    ${CMAKE_SOURCE_DIR}/components/Embedding/EmbeddingModel
    ${CMAKE_SOURCE_DIR}/components/Chat
    ${CMAKE_SOURCE_DIR}/components/Chat/ChatHistory
    ${CMAKE_SOURCE_DIR}/components/Chat/Message
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
    ${Python3_LIBRARIES}
    ${TORCH_LIBRARIES}
)

# -------------------------------------------------------
# === Pybind11 Modules ==================================
# -------------------------------------------------------
# RagPUREAI
pybind11_add_module(RagPUREAI ${RagPUREAI_BINDING_SRCS})
target_link_libraries(RagPUREAI PRIVATE RagPUREAILib)

# vectordb
pybind11_add_module(vectordb components/VectorDatabase/python/_vectordb.cpp)
target_link_libraries(vectordb PRIVATE
    -Wl,--whole-archive
    VectorDatabase
    -Wl,--no-whole-archive
)

# Disables LTO/IPO in the module to avoid ODR/refcount problems.

set_property(TARGET vectordb PROPERTY INTERPROCEDURAL_OPTIMIZATION FALSE)
target_compile_options(vectordb PRIVATE -fno-lto)
target_link_options(vectordb PRIVATE -fno-lto)

# .so output
set_target_properties(vectordb PROPERTIES
    OUTPUT_NAME "vectordb"
    LIBRARY_OUTPUT_DIRECTORY  "${PROJECT_BINARY_DIR}/python"
    ARCHIVE_OUTPUT_DIRECTORY  "${PROJECT_BINARY_DIR}/python"
    RUNTIME_OUTPUT_DIRECTORY  "${PROJECT_BINARY_DIR}/python"
)
