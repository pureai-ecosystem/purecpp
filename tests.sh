#!/bin/bash
base_dir=./build/Release/apps
chunk_base_dir=${base_dir}/chunk
clean_data_dir=${base_dir}/clean_data

hr="---------------------------"

export OPENAI_API_KEY=$(cat openai_api_key)

echo "${hr} building ${hr}"
cmake --build $(pwd)/build/Release --parallel $(nproc) --target all --

run_app() {
    app_base_dir=$1
    app_name=$2
    app_exe=${app_base_dir}/${app_name}/${app_name}
    shift 2

    echo "${hr} ${app_name} ${hr}"
    ${app_exe} "$@"
}

run_app "${chunk_base_dir}" chunk_count --count_unit "coordinated" --overlap 600 --count_threshold 2 --filenamepath ./txts/sample24.txt

run_app "${chunk_base_dir}" chunk_default --chunk_size 100 --overlap 20 --filenamepath ./txts/sample24.txt

run_app "${chunk_base_dir}" chunk_similarity --filenamepath ./txts/sample1.txt

run_app "${chunk_base_dir}" chunk_similarity --embedding_model openai --filenamepath ./txts/sample1.txt

run_app "${chunk_base_dir}" chunk_query --query "atomic models" --similarity_threshold 0.2434 --filenamepath ./txts/sample1.txt

run_app "${chunk_base_dir}" chunk_query --query "atomic models" --embedding_model openai --similarity_threshold 0.80 --filenamepath ./txts/sample1.txt

run_app "${clean_data_dir}" content_cleaner --custom_patterns "[cC]lick" "," --filenamepath ./txts/sample24.txt