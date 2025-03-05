# PureCPP

**PureCPP** is the C++ backend for architectural code that powers the RAG system.

---

## 📖 Documentation
For detailed installation and setup instructions, please refer to our official documentation:

🔗 [PureCPP Documentation](https://docs.puredocs.org/setup)

---

## 🚀 Contributing to PureCPP

We welcome contributions to **PureCPP**! If you would like to contribute, please read our contribution guide before opening an issue or submitting a pull request:

👉 [Contribution Guide](../community/CONTRIBUTING.md)

---

## 📌 Minimum Requirements

Ensure you have the following dependencies installed before building PureCPP:

- **GCC/G++** 13.1
- **CMake** 3.22
- **Conan** 2
- **Rust**

---

## ⚡ Quick Start with PIP

To quickly get started with **PureCPP**, follow the installation guide in our documentation:

📖 [Setup Guide](https://docs.puredocs.org/setup)

---

## 🔨 How to Build

The following commands execute the `cleand_and_build.sh` script, which performs the following actions:

- Creates the required folders for the project build.
- Runs **CMake** to configure and generate the build files.
- Creates the **build** folder, where the compiled files will be stored.
- Installs **Conan** dependencies, ensuring all necessary libraries and packages are available.

### Build Commands:
```bash
chmod +x cleand_and_build.sh
./cleand_and_build.sh
```

---

## 📥 Downloading Pre-trained Models

To use pre-trained models with **PureCPP**, you can download and convert them to **ONNX** format using the following commands:

```bash
python3 scripts/hf_model_to_onnx.py -m="dbmdz/bert-large-cased-finetuned-conll03-english" -o="bert-large-cased-finetuned-conll03-english"
python3 scripts/hf_model_to_onnx.py -m="sentence-transformers/all-MiniLM-L6-v2" -o="sentence-transformers/all-MiniLM-L6-v2"
```

---

## 🦀 Rust Installation

### Install Rust:
```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```

### Activate Rust Environment:
```bash
source ~/.cargo/env
```

---

## 🔗 Downloading Libtorch

Libtorch is required for running PyTorch models in **C++**. Follow the links below for installation:

- 📖 [Installing Libtorch](https://pytorch.org/cppdocs/installing.html)
- 📥 [Download Binaries](https://pytorch.org/get-started/locally/)

![libtorch-download](docs/libtorch-download.png)

### CPU Version Installation
```bash
libtorch_cpu_zip=libtorch-cxx11-abi-shared-with-deps-2.5.1+cpu.zip
libtorch_cpu_url=https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-2.5.1%2Bcpu.zip

wget ${libtorch_cpu_url} -O ${libtorch_cpu_zip}
mkdir -p ./libs/libtorch
unzip ${libtorch_cpu_zip} -d ./libs/libtorch
mv ./libs/libtorch/libtorch/ ./libs/libtorch/cpu
```

### CUDA Version Installation
```bash
libtorch_cuda_zip=libtorch-cxx11-abi-shared-with-deps-2.5.1+cu124.zip
libtorch_cuda_url=https://download.pytorch.org/libtorch/cu124/libtorch-cxx11-abi-shared-with-deps-2.5.1%2Bcu124.zip

wget ${libtorch_cuda_url} -O ${libtorch_cuda_zip}
mkdir -p ./libs/libtorch
unzip ${libtorch_cuda_zip} -d ./libs/libtorch
mv ./libs/libtorch/libtorch/ ./libs/libtorch/cuda
```

---

## 📌 Next Steps
![Next Steps](../community/release.jpg)

Stay tuned for updates! 🚀

