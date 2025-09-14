#!/bin/bash
set -e

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

#-----------------------------------------
ZIP=libtorch-cxx11-abi-shared-with-deps-2.5.0+cpu.zip
URL=https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-2.5.0%2Bcpu.zip

rm -rf ${ZIP} ./libs/libtorch/cpu
wget ${URL} -O ${ZIP}
mkdir -p ./libs/libtorch
unzip ${ZIP} -d ./libs/libtorch
mv ./libs/libtorch/libtorch ./libs/libtorch/cpu
#-----------------------------------------

#-----------------------------------------
#================= ENDING ================
#-----------------------------------------
printf "$SEGMENT$SEGMENT$SEGMENT"
printf "\n\n\n\n\n".
#-----------------------------------------