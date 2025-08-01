FROM python:3.12-slim AS builder

WORKDIR /app

RUN echo "deb http://deb.debian.org/debian testing main" > /etc/apt/sources.list.d/testing.list && \
    echo 'APT::Default-Release "bookworm";' > /etc/apt/apt.conf.d/00default-release && \
    apt-get update && \
    apt-get install -y -t testing \
      gcc-13 g++-13 libstdc++-13-dev && \
    apt-get install -y \
      git \
      curl \
      wget \
      cmake \
      nano \
      unzip \
      ninja-build \
      pkg-config \
      libffi-dev \
      libprotobuf-dev \
      protobuf-compiler \
      libgflags-dev \
      libssl-dev \
      sudo \
      build-essential \
      gnupg \
    && apt-get clean && rm -rf /var/lib/apt/lists/* && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100

COPY .git .git
COPY .gitmodules .gitmodules
COPY scripts/ ./scripts/
RUN chmod +x -R /app/scripts

RUN mkdir -p /app/libs/openai-cpp /app/libs/tokenizers-cpp

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
ENV PATH="/root/.cargo/bin:${PATH}"

RUN /app/scripts/install_python_dependencies.sh
RUN /app/scripts/install_torch.sh
RUN /app/scripts/install_libs.sh
RUN /app/scripts/configure_conan_profile.sh

# COPY . .

CMD ["/bin/bash"]
