#!/usr/bin/env bash
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


#-----------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

pip install build conan cmake requests pybind11

"$SCRIPT_DIR/setting_conan_profile.sh"


"$SCRIPT_DIR/torch_installer.sh"


"$SCRIPT_DIR/faiss_installer.sh"
#-----------------------------------------


#-----------------------------------------
#================= ENDING ================
#-----------------------------------------
printf "                   END\n"
printf "$SEGMENT$SEGMENT$SEGMENT\n"
#-----------------------------------------
