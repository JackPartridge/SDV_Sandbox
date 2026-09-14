FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive \
    VSOMEIP_VERSION=3.7.5 \
    VSOMEIP_CONFIGURATION=/workspace/config/vsomeipLocal.json

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    ca-certificates \
    pkg-config \
    libboost-system-dev \
    libboost-thread-dev \
    libboost-filesystem-dev \
    libboost-log-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /tmp

RUN git clone --depth 1 --branch ${VSOMEIP_VERSION} \
      https://github.com/COVESA/vsomeip.git \
    && cmake -S vsomeip -B vsomeip/build \
      -DCMAKE_BUILD_TYPE=Release \
      -DENABLE_SIGNAL_HANDLING=1 \
    && cmake --build vsomeip/build --parallel "$(nproc)" \
    && cmake --install vsomeip/build \
    && ldconfig \
    && rm -rf vsomeip

WORKDIR /workspace

COPY CMakeLists.txt ./
COPY src ./src
COPY tests ./tests
COPY examples ./examples
COPY config ./config
COPY scripts ./scripts

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --parallel "$(nproc)" \
    && ctest --test-dir build --output-on-failure \
    && chmod +x /workspace/scripts/runSandbox.sh \
               /workspace/scripts/runDemo.sh \
               /workspace/scripts/runCiHarness.sh \
               /workspace/scripts/runCanCiHarness.sh \
               /workspace/scripts/runCanGateway.sh

LABEL org.opencontainers.image.title="SDV vECU Sandbox" \
      org.opencontainers.image.description="Local SOME/IP publisher/subscriber sandbox for shift-left SDV testing"

CMD ["/workspace/scripts/runSandbox.sh"]
