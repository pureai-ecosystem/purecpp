#!/bin/bash
set -e

mkdir -p /root/.conan2/profiles

cat > /root/.conan2/profiles/default <<EOF
[settings]
arch=x86_64
build_type=Release
compiler=gcc
compiler.cppstd=20
compiler.libcxx=libstdc++11
compiler.version=13
os=Linux
EOF
