# Use the official manylinux image (compatible with Python packaging standards)
FROM quay.io/pypa/manylinux_2_28_x86_64

# Add Python 3.12 binaries to PATH (provided by the manylinux image)
ENV PATH="/opt/python/cp312-cp312/bin:${PATH}"

# Set working directory
WORKDIR /app

# Install GCC 13 and common development tools using YUM (not APT!)
RUN yum install -y \
    gcc gcc-c++ make cmake git curl wget \
    ninja-build libffi-devel openssl-devel \
    protobuf-devel gflags-devel zlib-devel \
    unzip nano \
    openblas-devel

# Install Rust (for building Rust-based Python extensions, if needed)
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y

# Add Rust binaries to PATH
ENV PATH="/root/.cargo/bin:${PATH}"

# Upgrade pip and install required Python packages globally
RUN python3 -m pip install --upgrade pip setuptools wheel && \
    pip install build conan cmake requests twine pybind11 numpy

# Install common ML and ONNX tooling
RUN pip install torch transformers onnx onnxruntime optimum

# Default shell for container
CMD ["/bin/bash"]
