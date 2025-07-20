#!/bin/bash

mkdir -p ../libs/tokenizers-cpp
mkdir -p ../libs/openai-cpp

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

#-----------------------------------------------------------------------
#-----------------------------------------------------------------------
