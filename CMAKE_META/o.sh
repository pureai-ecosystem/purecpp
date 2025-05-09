# #!/bin/bash

set -e
echo "PATH: $PATH"

rm -fr build conan.lock

conan install . --build=missing
conan lock create conanfile.py --build=missing
conan install . --build=missing

cmake -DCMAKE_POLICY_DEFAULT_CMP0091=NEW \
    -DBUILD_SHARED_LIBS=OFF \
    -D_GLIBCXX_USE_CXX11_ABI=1 \
    -DSPM_USE_BUILTIN_PROTOBUF=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=generators/conan_toolchain.cmake \
    -S "$(pwd)" \
    -B "$(pwd)/build/Release" \
    -G "Unix Makefiles"

cmake --build "$(pwd)/build/Release" --parallel "$(nproc)"
rm -f ../Tests/purecpp_meta*.so
cp ./build/Release/purecpp_meta*.so ../Tests/
