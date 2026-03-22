FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    python3 \
    python3-pip \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install --no-cache-dir conan

WORKDIR /app

COPY CMakeLists.txt ./
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

COPY --from=builder /app/build/backend /app/backend

ENV HOST=0.0.0.0
ENV PORT=8080
ENV HTTPS_ENABLED=false
ENV HTTPS_PORT=8443
ENV HTTPS_CERT_FILE=/certs/cert.pem
ENV HTTPS_KEY_FILE=/certs/key.pem

EXPOSE 8080 8443

CMD ["/app/backend"]
