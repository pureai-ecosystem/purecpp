#!/bin/bash
set -e
set -x 

rm -fr build conan.lock

conan lock create ./src --build=missing
conan install ./src --build=missing


cmake \
  --preset conan-release \
  -DCMAKE_POLICY_DEFAULT_CMP0091=NEW \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DSPM_USE_BUILTIN_PROTOBUF=OFF \
  -G "Unix Makefiles"

cmake --build --preset conan-release --parallel $(nproc) --target RagPUREAI --

rm -f ./Sandbox/*.so
# cp ./src/build/Release/$MOD*.so ./Sandbox/

