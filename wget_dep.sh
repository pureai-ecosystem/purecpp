#!/bin/bash

# export PATH="/opt/python/cp311-cp311/bin:$PATH"
# export PATH="/opt/python/cp312-cp312/bin:$PATH"

#-----------------------------------------------------------------------------------------------------------------------------------
git submodule update --init --recursive --remote
pushd libs/tokenizers-cpp
    git checkout 4bb7533
    git submodule update --init --recursive --remote
    pushd msgpack
        git checkout 8c602e8
    popd
    pushd sentencepiece
        git checkout f2219b5
    popd
popd

libtorch_cpu_zip=libtorch-cxx11-abi-shared-with-deps-2.5.0+cpu.zip
libtorch_cpu_url=https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-2.5.0%2Bcpu.zip
rm ${libtorch_cpu_zip} ./libs/libtorch/cpu -fr
wget ${libtorch_cpu_url} -O ${libtorch_cpu_zip}
mkdir -p ./libs/libtorch
unzip ${libtorch_cpu_zip} -d ./libs/libtorch
mv ./libs/libtorch/libtorch/ ./libs/libtorch/cpu

# libtorch_cpu_zip=libtorch-shared-with-deps-2.6.0%2Bcpu.zip
# libtorch_cpu_url=https://download.pytorch.org/libtorch/cpu/libtorch-shared-with-deps-2.6.0%2Bcpu.zip
# rm ${libtorch_cpu_zip} ./libs/libtorch/cpu -fr
# wget ${libtorch_cpu_url} -O ${libtorch_cpu_zip}
# mkdir -p ./libs/libtorch2
# unzip ${libtorch_cpu_zip} -d ./libs/libtorch
# mv ./libs/libtorch/libtorch/ ./libs/libtorch/cpu

#-----------------------------------------------------------------------------------------------------------------------------------

# rm models/ -fr
# python3 scripts/hf_model_to_onnx.py -m="sentence-transformers/all-MiniLM-L6-v2" -o="sentence-transformers/all-MiniLM-L6-v2"