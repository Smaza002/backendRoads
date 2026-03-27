FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    python3 \
    python3-pip \
    python3-venv \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

RUN python3 -m venv /opt/conan-venv
ENV PATH="/opt/conan-venv/bin:${PATH}"
RUN pip install --no-cache-dir --upgrade pip && \
    pip install --no-cache-dir conan

WORKDIR /app

COPY CMakeLists.txt ./
COPY conanfile.txt ./
COPY cmake ./cmake
COPY conan ./conan
COPY src ./src
COPY cots ./cots

RUN conan profile detect --force
RUN conan install . --output-folder=conan/generated --build=missing -s build_type=Release
RUN cmake -S . -B build -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=conan/generated/build/Release/generators/conan_toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build --config Release

FROM ubuntu:24.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    openssl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/build/src/backend /app/backend
COPY docker/entrypoint.sh /app/entrypoint.sh

RUN chmod +x /app/entrypoint.sh

ENV HOST=0.0.0.0
ENV PORT=8080
ENV HTTPS_ENABLED=true
ENV HTTPS_PORT=8443
ENV HTTPS_CERT_FILE=/app/certs/cert.pem
ENV HTTPS_KEY_FILE=/app/certs/key.pem

EXPOSE 8443

ENTRYPOINT ["/app/entrypoint.sh"]
