# PureCPP

**PureCPP** is the C++ backend powering the core logic of the RAG (Retrieval-Augmented Generation) system. It provides high-performance native modules that integrate seamlessly with Python via bindings.

## Contributing

We welcome contributions to **PureCPP**!

Before submitting a pull request or issue, please read our [Contribution Guide](/community/CONTRIBUTING.md).

## Project Structure

```
.
├── scripts/                 # Shell utilities and setup scripts
├── package/                 # Python package
│   └── purecpp/             # Contains the compiled .so
├── build/                   # Generated build files
├── libs/                    # Third-party dependencies
├── CMakeLists.txt           # Main build config
├── Dockerfile               # Build environment
└── README.md
```

## Documentation

For full installation and setup instructions, visit our official documentation:

🔗 [PureCPP Documentation](https://docs.puredocs.org/setup)

## Quick Start with PIP

To install the package via `pip` (for end-users):

```bash
pip install purecpp
```

## Build Options

You can either **build locally** or use our **Docker environment** to ensure consistency.

### Building with Docker (Recommended)

To simplify setup and avoid installing system-wide dependencies, use the provided Dockerfile.

#### Step 1: Build the Docker image

```bash
docker build -t purecpp .
```

#### Step 2: Start a bash shell inside the container

```bash
docker run -it --rm purecpp bash
```

#### Step 3: Build the project

```bash
./build.sh
```

This will generate the shared object (`RagPUREAI.cpython-<python-version>*.so`) in the `build/Release/` directory.

#### Step 4: Copy `.so` to your test folder

To test the Python bindings, copy the `.so` file to your test script directory:

```bash
cp build/Release/RagPUREAI*.so /some-test-folder
```

### Building Locally (Alternative)

You may also build the project manually without Docker, if your environment satisfies the requirements.

#### Minimum Requirements to Build Locally

* **Python** ≥ 3.8
* **CMake** ≥ 3.22
* **Conan** ≥ 2.0
* **Rust**
* **GCC/G++** = 13
* **Protobuf Compiler**

#### Build Steps

```bash
chmod +x scripts/install_python_dependencies.sh
chmod +x scripts/install_torch.sh
chmod +x scripts/install_libs.sh
chmod +x scripts/configure_conan_profile.sh
chmod +x build

# Install dependencies
./scripts/install_python_dependencies.sh
./scripts/install_torch.sh
./scripts/install_libs.sh
./scripts/configure_conan_profile.sh

# Build the project
./build
```

The output `.so` file will be located in `build/Release/`.

---

## Testing Locally

To test the Python bindings:

```python
from RagPUREAI import SomeExposedFunction 
```

Ensure `RagPUREAI*.so` is placed in the same folder as your Python project.

---

## Publishing to PyPI

To build and upload the Python package:

```bash
./scripts/create_pip_package <your-pypi-api-key>
```

This script will:

* Copy the `.so` file to the appropriate location.
* Package the Python module using `setuptools`.
* Upload to PyPI using `twine`.

---

## Downloading Pre-trained Models

You can convert HuggingFace models to ONNX using:

```bash
python3 scripts/hf_model_to_onnx.py -m="dbmdz/bert-large-cased-finetuned-conll03-english" -o="bert-large-cased-finetuned-conll03-english"
python3 scripts/hf_model_to_onnx.py -m="sentence-transformers/all-MiniLM-L6-v2" -o="sentence-transformers/all-MiniLM-L6-v2"
```

---

## Next Steps

![Next Steps](community/release.jpg)

Stay tuned for updates and new model integrations! 🚀
