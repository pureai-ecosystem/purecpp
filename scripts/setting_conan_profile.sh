#!/bin/bash
set -e

TAG="[$(basename "${BASH_SOURCE[0]}")]"
LINE_BRK="\n\n"
SEGMENT="===========================================================\n"

printf "$SEGMENT"
printf "$SEGMENT"
printf "$SEGMENT"
printf "                   $TAG"

#-----------------------------------------
printf "$LINE_BRK$SEGMENT"
printf "$TAG conan profile detect --force\n"
conan profile detect --force
#-----------------------------------------

#-----------------------------------------
printf "$LINE_BRK$SEGMENT"
printf "$TAG Finding\n"

PROFILE_DIR=$(find . -type d -wholename "*/.conan2/profiles" | head -n 1 || true)
[ -z "$PROFILE_DIR" ] && PROFILE_DIR="$HOME/.conan2/profiles" && mkdir -p "$PROFILE_DIR"

printf "$TAG Found at $PROFILE_DIR\n"

#-----------------------------------------

#-----------------------------------------
printf "$LINE_BRK$SEGMENT"
printf "$TAG Writing default profile$LINE_BRK"


# Old Setup (New was set to compiler.cppstd=20 and compiler.version=13. But was resulting in issues.)
cat << EOF > "$PROFILE_DIR/default"
[settings]
arch=x86_64
build_type=Release
compiler=gcc
compiler.cppstd=17
compiler.libcxx=libstdc++11
compiler.version=11
os=Linux
EOF

printf "$LINE_BRK$SEGMENT"
#-----------------------------------------

printf "$TAG Profile created in: $PROFILE_DIR/default\n"
printf "$TAG Checking: cat < $PROFILE_DIR/default $LINE_BRK"

cat < $PROFILE_DIR/default
printf "$LINE_BRK"

printf "$SEGMENT\n"

printf "\nHard-check with: cat < $PROFILE_DIR/default$LINE_BRK"

printf "$SEGMENT$SEGMENT$SEGMENT"
printf "\n\n\n\n\n"