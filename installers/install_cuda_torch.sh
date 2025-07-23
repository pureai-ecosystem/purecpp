#!/bin/bash

libtorch_cuda_zip=libtorch-cxx11-abi-shared-with-deps-2.5.1+cu124.zip
libtorch_cuda_url=https://download.pytorch.org/libtorch/cu124/libtorch-cxx11-abi-shared-with-deps-2.5.1%2Bcu124.zip

mkdir -p ../libs/libtorch/cuda

rm -fr ${libtorch_cuda_zip} ../libs/libtorch/cuda
rm -fr ${libtorch_cuda_zip} libtorch/ 
wget ${libtorch_cuda_url} -O ${libtorch_cuda_zip}

unzip ${libtorch_cuda_zip}
mv ./libtorch/* ../libs/libtorch/cuda
rm -fr libtorch/ 
