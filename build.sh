#!/usr/bin/env bash

set -euo pipefail

#-----------------------------------------
#================= LOGGING ===============
#-----------------------------------------
TAG="[$(basename "${BASH_SOURCE[0]}")]"
LINE_BRK=$'\n\n'
SEGMENT=$'===========================================================\n'
#-----------------------------------------

#-----------------------------------------
printf "$SEGMENT$SEGMENT$SEGMENT"
printf "              Begin $TAG$LINE_BRK"
printf "$SEGMENT"
printf "$LINE_BRK"
#-----------------------------------------



# ─────────────────────────────────────────────────────────────────────────────
# Conan
# ─────────────────────────────────────────────────────────────────────────────
#-----------------------------------------
printf "              Begin [CONAN]$LINE_BRK"
printf "$SEGMENT"
printf "$LINE_BRK"
#-----------------------------------------

rm -fr ./src/build ./src/conan.lock

conan lock create ./src --build=missing
conan install ./src --build=missing

#-----------------------------------------
#================= ENDING ================
#-----------------------------------------
printf "$SEGMENT"
printf "             Finish [CONAN]\n"
printf "$SEGMENT$SEGMENT$SEGMENT\n"
#-----------------------------------------



# ─────────────────────────────────────────────────────────────────────────────
# Build
# ─────────────────────────────────────────────────────────────────────────────
#-----------------------------------------
printf "              Begin [Build]$LINE_BRK"
printf "$SEGMENT"
printf "$LINE_BRK"
#-----------------------------------------
cd src/

cmake -DCMAKE_POLICY_DEFAULT_CMP0091=NEW \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DBUILD_SHARED_LIBS=OFF \
    -D_GLIBCXX_USE_CXX11_ABI=1 \
    -DSPM_USE_BUILTIN_PROTOBUF=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=generators/conan_toolchain.cmake \
    -S "$(pwd)" \
    -B "$(pwd)/build/Release" \
    -G "Unix Makefiles"

cmake --build "$(pwd)/build/Release" --parallel $(nproc)
# cmake --build --preset conan-release --parallel $(nproc) --target RagPUREAI 

#-----------------------------------------
#================= ENDING ================
#-----------------------------------------
printf "$SEGMENT"
printf "             Finish [Build]\n"
printf "$SEGMENT$SEGMENT$SEGMENT\n"
#-----------------------------------------



# ─────────────────────────────────────────────────────────────────────────────
# Sending to Sandbox
# ─────────────────────────────────────────────────────────────────────────────

printf "[Last Step] Sending to Sandbox \n"

rm -f ../Sandbox/*.so

cp ./build/Release/RagPUREAI.cpython*.so ../Sandbox/

#-----------------------------------------
#================= ENDING ================
#-----------------------------------------
printf "$SEGMENT"
printf "             Finish $TAG\n"
printf "$SEGMENT$SEGMENT$SEGMENT\n"
#-----------------------------------------
