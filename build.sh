#!/bin/bash
set -e
set -x 

sed -i s/compiler.version=.*/compiler.version=11/g ~/.conan2/profiles/default
conan install . --build=missing

cmake \
  --preset conan-release \
  -DCMAKE_POLICY_DEFAULT_CMP0091=NEW \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DSPM_USE_BUILTIN_PROTOBUF=OFF \
  -G "Unix Makefiles"
cmake --build --preset conan-release --parallel $(nproc) --target RagPUREAI --

rm -f package/purecpp/*.so
rm -rf package/dist/*

cp build/Release/*.so package/purecpp/

