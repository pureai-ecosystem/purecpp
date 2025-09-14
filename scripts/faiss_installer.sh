#!/usr/bin/env bash

#-----------------------------------------
#================= LOGGIN ================
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


# sudo se necessário
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

# Detecta gerenciador de pacotes
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

# Dependências
echo "[pkg] Installing build dependencies..."
if [[ "$PKG_MANAGER" == "apt" ]]; then
  $SUDO apt-get update -y
  $SUDO apt-get install -y \
    cmake g++ libopenblas-dev libgflags-dev build-essential \
    python3-dev git unzip wget pkg-config ninja-build
else
  $SUDO yum install -y \
    gcc gcc-c++ make cmake git curl wget ninja-build \
    libffi-devel openssl-devel protobuf-devel gflags-devel \
    zlib-devel unzip openblas-devel pkgconf-pkg-config
fi

# Pastas / TAG
PROJ_DIR="$(pwd)"
FAISS_DIR="${PROJ_DIR}/../libs/faiss"
FAISS_TAG="${FAISS_TAG:-v1.8.0}"

echo "[fs] Preparing ${FAISS_DIR} (fresh clone)"
rm -rf "$FAISS_DIR"
mkdir -p "$(dirname "$FAISS_DIR")"

# Clone
echo "[git] Cloning FAISS (${FAISS_TAG})..."
git clone --branch "$FAISS_TAG" --single-branch --depth 1 \
  https://github.com/facebookresearch/faiss.git "$FAISS_DIR"

cd "$FAISS_DIR"

# Ninja se disponível
GEN_ARGS=()
if command -v ninja >/dev/null 2>&1; then
  GEN_ARGS+=( -G Ninja )
fi

echo "[cmake] Configuring (CPU-only, Release)..."
cmake -B build "${GEN_ARGS[@]}" \
  -DFAISS_ENABLE_GPU=OFF \
  -DFAISS_ENABLE_PYTHON=OFF \
  -DBUILD_TESTING=OFF \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_STANDARD=17 \
  -DCMAKE_POLICY_DEFAULT_CMP0135=NEW

echo "[cmake] Building (target: faiss)..."
cmake --build build --target faiss --config Release --parallel "$(nproc)"

# Localiza artefatos
FOUND_LIB="$(find "$FAISS_DIR/build/faiss" -maxdepth 1 -name 'libfaiss.*' -print -quit 2>/dev/null || true)"

if [[ -n "${FOUND_LIB}" && -e "${FOUND_LIB}" ]]; then
  echo "[ok] FAISS built successfully."
  echo "[out] Headers : $FAISS_DIR/faiss/"
  echo "[out] Library : $FOUND_LIB"
else
  echo "[x] Build finished but libfaiss was not found under build/faiss/" >&2
  exit 2
fi

#-----------------------------------------
#================= ENDING ================
#-----------------------------------------
printf "$SEGMENT$SEGMENT$SEGMENT"
printf "\n\n\n\n\n"
#-----------------------------------------
