#!/usr/bin/env bash

# Script to install and build FAISS (CPU-only) for C++ usage
# Compatible with CentOS/RHEL systems using yum
# It clones FAISS into libs/faiss/ and builds it with CMake

set -euo pipefail

#-----------------------------------------
#================= LOGGING ===============
#-----------------------------------------

TAG="[$(basename "${BASH_SOURCE[0]}")]"
LINE_BRK="\n\n"
SEGMENT="===========================================================\n"

printf "$SEGMENT$SEGMENT$SEGMENT"
printf "                   $TAG$LINE_BRK"
printf "$SEGMENT"
printf "$LINE_BRK"
#-----------------------------------------

# ─────────────────────────────────────────────────────────────────────────────
# Elevation helper: use sudo only when needed and available
# ─────────────────────────────────────────────────────────────────────────────
SUDO=""
if [[ "$(id -u)" -ne 0 ]]; then
  if command -v sudo >/dev/null 2>&1; then
    SUDO="sudo"
  else
    echo "[!] Not running as root and 'sudo' is not available.
    Re-run as root or install sudo." >&2
    exit 1
  fi
fi



# ─────────────────────────────────────────────────────────────────────────────
# Detect package manager
# ─────────────────────────────────────────────────────────────────────────────
PKG_MANAGER=""
if command -v apt-get >/dev/null 2>&1; then
  PKG_MANAGER="apt"
  echo "[pkg] Detected APT-based system (Ubuntu/Debian)"
elif command -v yum >/dev/null 2>&1; then
  PKG_MANAGER="yum"
  echo "[pkg] Detected YUM-based system (manylinux/CentOS-like)"
else
  echo "[x] Unsupported system: neither apt-get nor yum found." >&2
  exit 1
fi



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


echo "[pkg] Installing required development packages..."
if [[ "$PKG_MANAGER" == "apt" ]]; then
  $SUDO apt-get update -y
  $SUDO apt install libgflags-dev -y
  $SUDO apt install -y cmake g++ libopenblas-dev python3-dev build-essential git
#   $SUDO apt-get install -y \
#     cmake g++ libopenblas-dev libgflags-dev build-essential \
#     python3-dev git unzip wget pkg-config ninja-build binutils
    
else
    echo "Checking if EPEL is installed..."
    if ! rpm -q epel-release >/dev/null 2>&1; then
        echo "Installing EPEL repository..."
        yum install -y epel-release
    fi

    $SUDO yum update -y
    $SUDO yum groupinstall -y "Development Tools"
    $SUDO yum install -y cmake3 gcc-c++ openblas-devel python3-devel git
    $SUDO yum install gflags-devel -y
#   $SUDO yum install -y \
#     gcc gcc-c++ cmake3 make cmake git curl wget ninja-build \
#     libffi-devel openssl-devel protobuf-devel gflags-devel \
#     zlib-devel unzip openblas-devel pkgconf-pkg-config binutils
fi


# Ensure `cmake` command exists, link it to `cmake3` if missing
if ! command -v cmake >/dev/null && command -v cmake3 >/dev/null; then
    echo "Linking cmake3 to cmake..."
    ln -s /usr/bin/cmake3 /usr/bin/cmake
fi



# ─────────────────────────────────────────────────────────────────────────────
# BUILD: Configure and compile FAISS (CPU-only)
# ─────────────────────────────────────────────────────────────────────────────

echo "Configuring CMake for CPU-only FAISS build..."
cmake -B build \
    -DFAISS_ENABLE_GPU=OFF \
    -DFAISS_ENABLE_PYTHON=OFF \
    -DFAISS_ENABLE_TESTS=OFF \
    -DCMAKE_BUILD_TYPE=Release

echo "Building FAISS..."

cmake --build build --parallel 3
# cmake --build build -j$(nproc)

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

#-----------------------------------------
#================= ENDING ================
#-----------------------------------------
printf "$SEGMENT$SEGMENT$SEGMENT"
printf "\n"
