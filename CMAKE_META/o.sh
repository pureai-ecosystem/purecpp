# #!/bin/bash

set -e
echo "PATH: $PATH"

rm -fr build conan.lock

#conan install . --build=missing
conan lock create conanfile.py --build=missing
conan install . --build=missing

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

cores=$(nproc)
if [ "$cores" -gt 1 ]; then
    half=$((cores / 2))
else
    half=1
fi
cmake --build "$(pwd)/build/Release" --parallel "$half"
rm -f ../Sandbox/purecpp_meta*.so
cp ./build/Release/purecpp_meta*.so ../Sandbox/
