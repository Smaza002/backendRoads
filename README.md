# C++20 Backend Scaffold

Backend en C++20 preparado para Render con la misma logica base que tenia el backend anterior:

- `POST /api/auth/register`
- `POST /api/auth/login`
- `GET /api/auth/me`
- PostgreSQL por `DATABASE_URL`
- JWT por `JWT_SECRET`

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

## Variables de entorno

- `HOST`: bind del servidor. Por defecto `0.0.0.0`
- `PORT`: puerto HTTP. Por defecto `8080`
- `DATABASE_URL`: cadena de conexion PostgreSQL
- `JWT_SECRET`: secreto para firmar tokens

## Notas de base de datos

El backend crea `pgcrypto` al arrancar con `CREATE EXTENSION IF NOT EXISTS pgcrypto` para poder hashear y validar passwords con bcrypt desde PostgreSQL.
Tu tabla `users` debe tener al menos:

```sql
CREATE TABLE IF NOT EXISTS users (
  id SERIAL PRIMARY KEY,
  email TEXT UNIQUE NOT NULL,
  password TEXT NOT NULL
);
```

## Desarrollo local

```bash
conan profile detect --force
conan install . --output-folder=conan/generated --build=missing -s build_type=Release
cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=conan/generated/build/Release/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/backend
```
