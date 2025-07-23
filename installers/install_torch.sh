#!/bin/bash

libtorch_cpu_zip=libtorch-cxx11-abi-shared-with-deps-2.5.0+cpu.zip

libtorch_cpu_url=https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-2.5.0%2Bcpu.zip


if [ -d ../libs/libtorch/cpu ]; then
    echo "Cleaning ../libs/libtorch/cpu/"
    rm -fr ../libs/libtorch/cpu/ 
else
    echo "../libs/libtorch/cpu/"
    mkdir -p ../libs/libtorch/cpu 
fi

rm -fr libtorch/ 
wget ${libtorch_cpu_url} -O ${libtorch_cpu_zip}

unzip ${libtorch_cpu_zip}
mv ./libtorch/* ../libs/libtorch/cpu 
rm -fr libtorch/ 