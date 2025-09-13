#!/usr/bin/env bash

# =============================================================================
# FAISS CPU Installer Script (C++ only)
# -----------------------------------------------------------------------------
# Works on Ubuntu/Debian (APT) and manylinux/CentOS-like (YUM) by auto-detecting
# the package manager. It installs build deps and builds FAISS (CPU-only) into
# ../libs/faiss relative to the current working directory.
# -----------------------------------------------------------------------------
# Usage (optional):
#   FAISS_TAG=v1.8.0 ./install_faiss_cpu.sh   # pin to a tag/branch (default v1.8.0)
# =============================================================================

set -euo pipefail

# ─────────────────────────────────────────────────────────────────────────────
# Elevation helper: use sudo only when needed and available
# ─────────────────────────────────────────────────────────────────────────────
SUDO=""
if [[ "$(id -u)" -ne 0 ]]; then
  if command -v sudo >/dev/null 2>&1; then
    SUDO="sudo"
  else
    echo "[!] Not running as root and 'sudo' is not available.\n    Re-run as root or install sudo." >&2
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
# Install dependencies
# ─────────────────────────────────────────────────────────────────────────────
echo "[pkg] Installing build dependencies..."
if [[ "$PKG_MANAGER" == "apt" ]]; then
  $SUDO apt-get update -y
  $SUDO apt-get install -y \
    cmake \
    g++ \
    libopenblas-dev \
    libgflags-dev \
    build-essential \
    python3-dev \
    git \
    unzip \
    wget \
    pkg-config \
    ninja-build
else
  # manylinux/CentOS-like
  $SUDO yum install -y \
    gcc \
    gcc-c++ \
    make \
    cmake \
    git \
    curl \
    wget \
    ninja-build \
    libffi-devel \
    openssl-devel \
    protobuf-devel \
    gflags-devel \
    zlib-devel \
    unzip \
    openblas-devel \
    pkgconf-pkg-config
fi

# ─────────────────────────────────────────────────────────────────────────────
# Prepare destination
# ─────────────────────────────────────────────────────────────────────────────
PROJ_DIR="$(pwd)"
FAISS_DIR="${PROJ_DIR}/../libs/faiss"
FAISS_TAG="${FAISS_TAG:-v1.8.0}"

echo "[fs] Preparing ${FAISS_DIR} (fresh clone)"
rm -rf "$FAISS_DIR"
mkdir -p "$(dirname "$FAISS_DIR")"

# ─────────────────────────────────────────────────────────────────────────────
# Clone & build (CPU-only)
# ─────────────────────────────────────────────────────────────────────────────
echo "[git] Cloning FAISS (${FAISS_TAG})..."
git clone --branch "$FAISS_TAG" --depth 1 https://github.com/facebookresearch/faiss.git "$FAISS_DIR"

cd "$FAISS_DIR"

# Prefer Ninja if available for faster builds
GEN_ARGS=()
if command -v ninja >/dev/null 2>&1; then
  GEN_ARGS+=( -G Ninja )
fi

echo "[cmake] Configuring (CPU-only, Release)..."
cmake -B build "${GEN_ARGS[@]}" \
  -DFAISS_ENABLE_GPU=OFF \
  -DFAISS_ENABLE_PYTHON=OFF \
  -DFAISS_ENABLE_TESTS=OFF \
  -DCMAKE_BUILD_TYPE=Release

echo "[cmake] Building..."
cmake --build build --config Release --parallel "$(nproc)"

# ─────────────────────────────────────────────────────────────────────────────
# Locate artifacts
# ─────────────────────────────────────────────────────────────────────────────
FOUND_LIB="$(find "$FAISS_DIR/build/faiss" -maxdepth 1 -name 'libfaiss.*' -print -quit 2>/dev/null || true)"

if [[ -n "${FOUND_LIB}" && -e "${FOUND_LIB}" ]]; then
  echo "[ok] FAISS built successfully."
  echo "[out] Headers : $FAISS_DIR/faiss/"
  echo "[out] Library : $FOUND