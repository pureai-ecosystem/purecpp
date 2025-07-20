# PureCPP

**PureCPP** is the C++ backend for architectural code that powers the RAG system.

---

## 📖 Documentation
For detailed installation and setup instructions, please refer to our official documentation:

🔗 [PureCPP Documentation](https://docs.puredocs.org/setup)

---

## 🚀 Contributing to PureCPP

We welcome contributions to **PureCPP**! If you would like to contribute, please read our contribution guide before opening an issue or submitting a pull request:

👉 [Contribution Guide](/community/CONTRIBUTING.md)

---

## 📌 Minimum Requirements

Ensure you have the following dependencies installed before building PureCPP:

- **GCC/G++** 13.1
- **CMake** 3.22
- **Conan** 2
- **Rust**

---
# Environment

# Ubuntu / Debian Setup for C++ and Python Development

## 1. Clone the Repository

```bash
git clone https://github.com/pureai-ecosystem/purecpp.git
cd purecpp
```

## 2. Installing dependencies

Run the following commands:

```bash
sudo apt update && \
sudo apt upgrade -y && \
sudo apt install -y \
    build-essential \
    nano \
    wget \
    curl \
    git \
    ninja-build \
    cmake \
    libopenblas-dev \
    libgflags-dev \
    python3-dev \
    libprotobuf-dev \
    protobuf-compiler
```

---

## 3. Install python essential packages

```bash
pip install build conan cmake requests pybind11
```

---

## 4. Execute the FAISS installation script

```bash
./installers/install_faiss_ubuntu.sh
```

---

## 5. Install Rust via rustup

### Run rustup installer non-interactively (-y). This places cargo and rustc in /root/.cargo.
```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
```

### Activate Rust Environment:
```bash
source ~/.cargo/env
```

---

## 6. Setting the Default Conan Profile 

To locate the path of the `.conan2/profiles/default` file, run:

```bash
find ~ -type d -wholename "*/.conan2/profiles"
````

Once you have located the `default` profile, open the file and edit it to include the following content:
```
[settings]
arch=x86_64
build_type=Release
compiler=gcc
compiler.cppstd=17
compiler.libcxx=libstdc++11
compiler.version=11
os=Linux
```

---

## 7. Use pre-trained models

### With **PureCPP**, you can download and convert them to **ONNX** format using the following commands:

```bash
python3 scripts/hf_model_to_onnx.py -m="dbmdz/bert-large-cased-finetuned-conll03-english" -o="bert-large-cased-finetuned-conll03-english"
python3 scripts/hf_model_to_onnx.py -m="sentence-transformers/all-MiniLM-L6-v2" -o="sentence-transformers/all-MiniLM-L6-v2"
```

**Notes:**

* Make sure to adjust `compiler.version` and `os` if your environment is different.
* This configuration ensures compatibility with GCC 11 and C++17 using the GNU libstdc++11 ABI.

```

