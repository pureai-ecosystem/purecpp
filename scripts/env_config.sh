#!/usr/bin/env bash
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

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


pip install build conan cmake requests pybind11

"$SCRIPT_DIR/setting_conan_profile.sh"


"$SCRIPT_DIR/torch_installer.sh"


"$SCRIPT_DIR/faiss_installer.sh"



#-----------------------------------------
#================= ENDING ================
#-----------------------------------------
printf "$SEGMENT$SEGMENT$SEGMENT"
printf "\n\n\n\n\n".
#-----------------------------------------