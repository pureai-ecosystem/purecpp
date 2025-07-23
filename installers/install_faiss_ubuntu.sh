#!/bin/bash

# Script to install and build FAISS (CPU-only) for C++ usage
# Compatible with Ubuntu/Debian-based systems using apt
# It clones FAISS into libs/faiss/ and builds it with CMake

if [ "$(id -u)" -ne 0 ]; then
    echo "🔒 Not running as root — using  sudo..."
    SUDO= sudo
else
    echo "🧙 Running as root —  sudo not needed."
    SUDO=""
fi

set -e  # Exit immediately if a command fails

# Project root is assumed to be the current directory
PROJ_DIR=$(pwd)

# Destination directory for FAISS
FAISS_DIR="${PROJ_DIR}/../libs/faiss"

echo "Creating libs/faiss/ directory inside the project..."
mkdir -p "$FAISS_DIR"

echo "Updating system packages..."
 $SUDO apt update

echo "Installing required development packages..."
 $SUDO apt install libgflags-dev -y
 $SUDO apt install -y cmake g++ libopenblas-dev python3-dev build-essential git

echo "Removing all files in repository $FAISS_DIR..."
rm -fr "$FAISS_DIR"

echo "Cloning FAISS repository into $FAISS_DIR..."
git clone https://github.com/facebookresearch/faiss.git "$FAISS_DIR"

cd "$FAISS_DIR"

echo "Configuring CMake for CPU-only FAISS build..."
cmake -B build -DFAISS_ENABLE_GPU=OFF -DFAISS_ENABLE_PYTHON=OFF -DFAISS_ENABLE_TESTS=OFF -DCMAKE_BUILD_TYPE=Release

echo "Building FAISS..."
cmake --build build --parallel 3

echo "FAISS has been successfully built."

# Find the libfaiss library (static or shared)
FOUND_LIB=$(find "$FAISS_DIR/build/faiss" -name "libfaiss.*" | head -n 1)

if [ -f "$FOUND_LIB" ]; then
    echo "Header files located at: $FAISS_DIR/faiss/"
    echo "Library file found at:   $FOUND_LIB"
else
    echo "Warning: libfaiss was not found in the expected directory."
fi

echo ""
echo "You can now link FAISS in your C++ project using:"
echo ""
echo '  include_directories(${CMAKE_SOURCE_DIR}/libs/faiss/faiss)'
echo '  link_directories(${CMAKE_SOURCE_DIR}/libs/faiss/build/faiss)'
echo '  target_link_libraries(your_target PRIVATE faiss)'

