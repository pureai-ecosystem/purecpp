#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
# set -e

# # Enable debug mode to print each command.
# set -x

# # Define variables
BUILD_DIR="build"
LIBTORCH_DIR="./libs/libtorch"
LIBTORCH_CPU_ZIP="libtorch-cxx11-abi-shared-with-deps-2.5.1+cpu.zip"
LIBTORCH_CPU_URL="https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-2.5.1%2Bcpu.zip"

# # Cleanup previous builds and lock files
# rm -rf ${BUILD_DIR} conan.lock

# # Initialize and update submodules
# git submodule update --init --recursive --remote

# # Update tokenizers-cpp submodule and dependencies
# pushd libs/tokenizers-cpp
#     git checkout 4bb7533
#     git submodule update --init --recursive --remote
#     pushd msgpack
#         git checkout 8c602e8
#     popd
#     pushd sentencepiece
#         git checkout f2219b5
#     popd
# popd

# Download and unpack libtorch
# rm -rf ${LIBTORCH_CPU_ZIP} ${LIBTORCH_DIR}/cpu
# wget ${LIBTORCH_CPU_URL} -O ${LIBTORCH_CPU_ZIP}
# mkdir -p ${LIBTORCH_DIR}
# unzip ${LIBTORCH_CPU_ZIP} -d ${LIBTORCH_DIR}
# mv ${LIBTORCH_DIR}/libtorch ${LIBTORCH_DIR}/cpu

# Uncomment below lines if model conversion is needed
# rm -rf models/
# python3 scripts/hf_model_to_onnx.py -m="sentence-transformers/all-MiniLM-L6-v2" -o="sentence-transformers/all-MiniLM-L6-v2"

# Run Conan package manager for dependency installation
rm -rf ${BUILD_DIR} conan.lock
conan install . --build=missing
conan lock create conanfile.py --build=missing
conan install . --build=missing

cmake \
    --preset conan-release \
    -DCMAKE_POLICY_DEFAULT_CMP0091=NEW \
    -DSPM_USE_BUILTIN_PROTOBUF=OFF \
    -G "Unix Makefiles"
cmake --build --preset conan-release --parallel $(nproc) --target RagPUREAI --