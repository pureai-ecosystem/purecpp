# Use the official manylinux image (compatible with Python packaging standards)
FROM quay.io/pypa/manylinux_2_28_x86_64

# Add Python 3.12 binaries to PATH
ENV PATH="/opt/python/cp312-cp312/bin:${PATH}"

# Set working directory
WORKDIR /app

# Install development tools, Python deps, Rust, and cleanup to save space
RUN yum install -y \
      gcc gcc-c++ make git curl wget \
      ninja-build libffi-devel openssl-devel \
      protobuf-devel gflags-devel zlib-devel \
      openblas-devel unzip\
  && curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y \
  && yum clean all \
  && rm -rf /var/cache/yum

# Add Rust to PATH
ENV PATH="/root/.cargo/bin:${PATH}"

# Upgrade pip & install Python build tools and ML packages
RUN python3 -m pip install --upgrade pip setuptools wheel \
  && pip install --no-cache-dir \
      build conan cmake requests \
      pybind11 numpy \
      torch transformers \
      onnx onnxruntime optimum

# Set default shell
CMD ["/bin/bash"]
