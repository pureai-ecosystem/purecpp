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

#-----------------------------------------
#================= LOGGING ===============
#-----------------------------------------
set -euo pipefail

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
# Install dependencies
# ─────────────────────────────────────────────────────────────────────────────
echo "[pkg] Installing build dependencies..."
if [[ "$PKG_MANAGER" == "apt" ]]; then
  $SUDO apt-get update -y
  $SUDO apt-get install -y \
    cmake g++ libopenblas-dev libgflags-dev build-essential \
    python3-dev git unzip wget pkg-config ninja-build binutils
else
  $SUDO yum install -y \
    gcc gcc-c++ make cmake git curl wget ninja-build \
    libffi-devel openssl-devel protobuf-devel gflags-devel \
    zlib-devel unzip openblas-devel pkgconf-pkg-config binutils
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
git clone --branch "$FAISS_TAG" --single-branch --depth 1 \
  https://github.com/facebookresearch/faiss.git "$FAISS_DIR"

cd "$FAISS_DIR"

# Prefer Ninja if available for faster builds
GEN_ARGS=()
if command -v ninja >/dev/null 2>&1; then
  GEN_ARGS+=( -G Ninja )
fi

# Build/Install Toggles
BUILD_SHARED="${BUILD_SHARED:-OFF}"              # export BUILD_SHARED=ON para .so
DO_INSTALL="${DO_INSTALL:-ON}"                   # export DO_INSTALL=OFF para pular install
INSTALL_PREFIX="${INSTALL_PREFIX:-${FAISS_DIR}/_install}"

# Parallelism with fallback
JOBS="$( (command -v nproc >/dev/null && nproc) || getconf _NPROCESSORS_ONLN || echo 2 )"

echo "[cmake] Configuring (CPU-only, Release)..."
cmake -B build "${GEN_ARGS[@]}" \
  -DBUILD_SHARED_LIBS="${BUILD_SHARED}" \
  -DFAISS_ENABLE_GPU=OFF \
  -DFAISS_ENABLE_PYTHON=OFF \
  -DBUILD_TESTING=OFF \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
  -DCMAKE_CXX_STANDARD=17 \
  -DCMAKE_POLICY_DEFAULT_CMP0135=NEW \
  -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}"

echo "[cmake] Building (target: faiss) with ${JOBS} jobs..."
cmake --build build --target faiss --config Release --parallel "${JOBS}"

# Optional installation (generates include/ and lib/ and package config)
if [[ "${DO_INSTALL}" == "ON" ]]; then
  echo "[cmake] Installing to ${INSTALL_PREFIX}..."
  cmake --install build --component faiss 2>/dev/null || cmake --install build
fi

# ─────────────────────────────────────────────────────────────────────────────
# Locate artifacts
# ─────────────────────────────────────────────────────────────────────────────
if [[ "${BUILD_SHARED}" == "ON" ]]; then
  PREFERRED="libfaiss.so"
  FALLBACK="libfaiss.a"
else
  PREFERRED="libfaiss.a"
  FALLBACK="libfaiss.so"
fi

FOUND_LIB="$(find "$FAISS_DIR/build/faiss" -maxdepth 1 -name "${PREFERRED}" -print -quit 2>/dev/null || true)"
if [[ -z "${FOUND_LIB}" ]]; then
  FOUND_LIB="$(find "$FAISS_DIR/build/faiss" -maxdepth 1 -name "${FALLBACK}" -print -quit 2>/dev/null || true)"
fi

if [[ -n "${FOUND_LIB}" && -e "${FOUND_LIB}" ]]; then
  echo "[ok] FAISS built successfully."
  echo "[out] Headers : $FAISS_DIR/faiss/"
  echo "[out] Library : $FOUND_LIB"
  if [[ "${DO_INSTALL}" == "ON" ]]; then
    echo "[out] Install  : ${INSTALL_PREFIX}"
    echo "       include : ${INSTALL_PREFIX}/include"
    echo "       lib     : ${INSTALL_PREFIX}/lib"
  fi
else
  echo "[x] Build finished but libfaiss was not found under build/faiss/" >&2
  exit 2
fi

# Useful post-build checks
if command -v nm >/dev/null 2>&1; then
  echo "[check] nm symbols (grep faiss::IndexFlat...):"
  nm -C "${FOUND_LIB}" | grep -E 'faiss::IndexFlat' | head || true
fi
if [[ "${FOUND_LIB##*.}" == "so" ]] && command -v ldd >/dev/null 2>&1; then
  echo "[check] ldd on shared library:"
  ldd "${FOUND_LIB}" || true
fi

#-----------------------------------------
#================= ENDING ================
#-----------------------------------------
printf "$SEGMENT$SEGMENT$SEGMENT"
printf "\n"
