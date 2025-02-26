#  RagPUREAI
RagPUREAI is the C++ backend for architectural code that will feed the RAG

## Minimum Requirements

* GCC/G++ 13.1
* CMAKE 3.22
* CONAN 2
* RUST

# How to build
The following command executes the cleand_and_build.sh script, which performs the following actions:

- Creates a folder required for the project build.
- Runs CMake to configure and generate the build files.
- Creates the build folder, where the compiled files will be stored.
- Installs Conan dependencies, ensuring all necessary libraries and packages are available.
```bash
chmod +x cleand_and_build.sh
./cleand_and_build.sh
```

# Download Model
```bash
python3 scripts/hf_model_to_onnx.py -m="dbmdz/bert-large-cased-finetuned-conll03-english" -o="bert-large-cased-finetuned-conll03-english"
python3 scripts/hf_model_to_onnx.py -m="sentence-transformers/all-MiniLM-L6-v2" -o="sentence-transformers/all-MiniLM-L6-v2"
```

# Rust

### Install Rust
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh

### Active Rust
source ~/.cargo/env

# Download libtorch

[Installing](https://pytorch.org/cppdocs/installing.html)

[Download binaries](https://pytorch.org/get-started/locally/)

![libtorch-download](docs/libtorch-download.png)

### CPU
```bash
libtorch_cpu_zip=libtorch-cxx11-abi-shared-with-deps-2.5.1+cpu.zip
libtorch_cpu_url=https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-2.5.1%2Bcpu.zip

wget ${libtorch_cpu_url} -O ${libtorch_cpu_zip}
mkdir -p ./libs/libtorch
unzip ${libtorch_cpu_zip} -d ./libs/libtorch
mv ./libs/libtorch/libtorch/ ./libs/libtorch/cpu
```

### Cuda

```bash
libtorch_cuda_zip=libtorch-cxx11-abi-shared-with-deps-2.5.1+cu124.zip
libtorch_cuda_url=https://download.pytorch.org/libtorch/cu124/libtorch-cxx11-abi-shared-with-deps-2.5.1%2Bcu124.zip

wget ${libtorch_cuda_url} -O ${libtorch_cuda_zip}
mkdir -p ./libs/libtorch
unzip ${libtorch_cuda_zip} -d ./libs/libtorch
mv ./libs/libtorch/libtorch/ ./libs/libtorch/cuda
```
