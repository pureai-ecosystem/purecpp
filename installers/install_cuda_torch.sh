#!/bin/bash

libtorch_cuda_zip=libtorch-cxx11-abi-shared-with-deps-2.5.1+cu124.zip
libtorch_cuda_url=https://download.pytorch.org/libtorch/cu124/libtorch-cxx11-abi-shared-with-deps-2.5.1%2Bcu124.zip

wget ${libtorch_cuda_url} -O ${libtorch_cuda_zip}
mkdir -p ./libs/libtorch
unzip ${libtorch_cuda_zip} -d ./libs/libtorch
mv ./libs/libtorch/libtorch/ ./libs/libtorch/cuda