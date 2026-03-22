# C++20 Backend Scaffold

Base backend en C++20 preparada para:

- CMake
- Conan 2
- Docker
- despliegue en Render

## Estructura

```text
.
├── CMakeLists.txt
├── Dockerfile
├── README.md
├── build/
├── cmake/
├── conan/
├── cots/
├── legacy-node/
└── src/
```

## Desarrollo local

```bash
conan profile detect --force
conan install . --output-folder=conan/generated --build=missing -s build_type=Release
cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=conan/generated/build/Release/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/backend
```

## Variables de entorno

- `PORT`: puerto HTTP del servidor. Por defecto `8080`.
- `HOST`: host bind. Por defecto `0.0.0.0`.

## Render

Configura el servicio como `Docker`.
La app escucha en `PORT` si Render lo inyecta.
