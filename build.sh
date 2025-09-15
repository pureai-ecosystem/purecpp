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

cmake \
  --preset conan-release \
  -DCMAKE_POLICY_DEFAULT_CMP0091=NEW \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DSPM_USE_BUILTIN_PROTOBUF=OFF \
  -G "Unix Makefiles"

cmake --build --preset conan-release --parallel $(nproc) --target RagPUREAI --
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

# cp ./src/build/Release/.so ../Sandbox/

#-----------------------------------------
#================= ENDING ================
#-----------------------------------------
printf "$SEGMENT"
printf "             Finish $TAG\n"
printf "$SEGMENT$SEGMENT$SEGMENT\n"
#-----------------------------------------
