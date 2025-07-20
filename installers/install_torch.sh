#!/bin/bash

libtorch_cpu_zip=libtorch-cxx11-abi-shared-with-deps-2.5.0+cpu.zip
libtorch_cpu_url=https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-2.5.0%2Bcpu.zip
rm ${libtorch_cpu_zip} ../libs/libtorch/cpu -fr
wget ${libtorch_cpu_url} -O ${libtorch_cpu_zip}
mkdir -p ../libs/libtorch
unzip ${libtorch_cpu_zip} -d ../libs/libtorch
mv ./libs/libtorch/libtorch/ ../libs/libtorch/cpu