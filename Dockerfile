FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    libabsl-dev \
    libbenchmark-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

COPY . .

ARG PXHASH_BUILD_BENCHMARKS=ON

RUN cmake -S . -B build -DPXHASH_BUILD_BENCHMARKS=${PXHASH_BUILD_BENCHMARKS} \
    && cmake --build build -j"$(nproc)"

CMD ["ctest", "--test-dir", "build", "--output-on-failure"]
