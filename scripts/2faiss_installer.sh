#!/bin/bash

# Script to install and build FAISS (CPU-only) for C++ usage
# Compatible with CentOS/RHEL systems using yum
# It clones FAISS into libs/faiss/ and builds it with CMake

set -e  # Exit immediately if a command fails
# ─────────────────────────────────────────────────────────────────────────────
# SETUP: Define directories
# ─────────────────────────────────────────────────────────────────────────────

# Assume the current directory is the project root
PROJ_DIR=$(pwd)

# Destination directory for FAISS
FAISS_DIR="${PROJ_DIR}/libs/faiss"

echo "Creating libs/faiss/ directory inside the project..."
mkdir -p "$FAISS_DIR"



# ─────────────────────────────────────────────────────────────────────────────
# SYSTEM: Update packages and install dependencies
# ─────────────────────────────────────────────────────────────────────────────

echo "Updating system packages..."
yum update -y

echo "Checking if EPEL is installed..."
if ! rpm -q epel-release >/dev/null 2>&1; then
    echo "Installing EPEL repository..."
    yum install -y epel-release
fi

echo "Installing required development packages..."
yum groupinstall -y "Development Tools"
yum install -y cmake3 gcc-c++ openblas-devel python3-devel git
yum install gflags-devel -y

# Ensure `cmake` command exists, link it to `cmake3` if missing
if ! command -v cmake >/dev/null && command -v cmake3 >/dev/null; then
    echo "Linking cmake3 to cmake..."
    ln -s /usr/bin/cmake3 /usr/bin/cmake
fi



# ─────────────────────────────────────────────────────────────────────────────
# CLONE: Download FAISS repository
# ─────────────────────────────────────────────────────────────────────────────

echo "Removing all files in repository $FAISS_DIR..."
rm -fr "$FAISS_DIR"

echo "Cloning FAISS repository into $FAISS_DIR..."
git clone https://github.com/facebookresearch/faiss.git "$FAISS_DIR"

cd "$FAISS_DIR"



# ─────────────────────────────────────────────────────────────────────────────
# BUILD: Configure and compile FAISS (CPU-only)
# ─────────────────────────────────────────────────────────────────────────────

echo "Configuring CMake for CPU-only FAISS build..."
cmake -B build -DFAISS_ENABLE_GPU=OFF -DFAISS_ENABLE_PYTHON=OFF -DFAISS_ENABLE_TESTS=OFF -DCMAKE_BUILD_TYPE=Release

echo "Building FAISS..."
cmake --build build --parallel 3

echo "FAISS has been successfully built."



# ─────────────────────────────────────────────────────────────────────────────
# VERIFY: Locate compiled library and headers
# ─────────────────────────────────────────────────────────────────────────────

# Find the libfaiss library (static or shared)
FOUND_LIB=$(find "$FAISS_DIR/build/faiss" -name "libfaiss.*" | head -n 1)

if [ -f "$FOUND_LIB" ]; then
    echo "Header files located at: $FAISS_DIR/faiss/"
    echo "Library file found at:   $FOUND_LIB"
else
    echo "Warning: libfaiss was not found in the expected directory."
fi



# ─────────────────────────────────────────────────────────────────────────────
# INFO: How to link FAISS in your C++ CMake project
# ─────────────────────────────────────────────────────────────────────────────

echo ""
echo "You can now link FAISS in your C++ project using:"
echo ""
echo '  include_directories(${CMAKE_SOURCE_DIR}/libs/faiss/faiss)'
echo '  link_directories(${CMAKE_SOURCE_DIR}/libs/faiss/build/faiss)'
echo '  target_link_libraries(your_target PRIVATE faiss)'

# cd libs/faiss
# cmake -B build -DFAISS_ENABLE_PYTHON=OFF -DFAISS_ENABLE_GPU=OFF
# cmake --build build -j$(nproc)