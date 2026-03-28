# C++20 Auth Backend

Backend de autenticacion en C++20 con arquitectura orientada a clases, PostgreSQL, JWT, CMake, Conan y tests con GoogleTest.

## Funcionalidad

- `POST /api/auth/register`
- `POST /api/auth/login`
- `GET /api/auth/me`
- PostgreSQL por `DATABASE_URL`
- JWT firmado con `JWT_SECRET`
- HTTPS en Docker con certificado autogenerado si no existe uno montado

## Arquitectura

La aplicacion ya no esta montada como funciones sueltas. La logica se reparte en clases e interfaces para que sea mas mantenible y testeable.

- [AppConfig](/home/desa/Proyectos/backend/src/config/app_config.hpp): lee y valida configuracion desde el entorno
- [Database](/home/desa/Proyectos/backend/src/config/database.hpp): encapsula la conexion y la inicializacion de extensiones
- [UserRepository](/home/desa/Proyectos/backend/src/repositories/user_repository.hpp): contrato para acceso a usuarios
- [PqxxUserRepository](/home/desa/Proyectos/backend/src/repositories/pqxx_user_repository.hpp): implementacion real sobre PostgreSQL/libpqxx
- [TokenService](/home/desa/Proyectos/backend/src/services/token_service.hpp): contrato para emision y validacion de tokens
- [JwtTokenService](/home/desa/Proyectos/backend/src/services/jwt_token_service.hpp): implementacion JWT con OpenSSL
- [AuthService](/home/desa/Proyectos/backend/src/services/auth_service.hpp): caso de uso de registro y login
- [AuthController](/home/desa/Proyectos/backend/src/controllers/auth_controller.hpp): adaptador HTTP sobre `cpp-httplib`

El target principal de la logica es `backend_core`, y el ejecutable final es `backend`.

## Estructura

```text
.
├── CMakeLists.txt
├── Dockerfile
├── docker-compose.yml
├── conanfile.txt
├── cmake/
├── docker/
├── src/
│   ├── config/
│   ├── controllers/
│   ├── repositories/
│   ├── services/
│   └── utils/
└── tests/
```

## Variables de entorno

- `HOST`: bind del servidor. Por defecto `0.0.0.0`
- `PORT`: puerto HTTP. Por defecto `8080`
- `HTTPS_ENABLED`: activa HTTPS. Valores truthy: `1`, `true`, `yes`, `on`
- `HTTPS_PORT`: puerto HTTPS. Por defecto `8443`
- `HTTPS_CERT_FILE`: ruta del certificado
- `HTTPS_KEY_FILE`: ruta de la clave privada
- `DATABASE_URL`: cadena de conexion PostgreSQL usada por la aplicacion
- `MIGRATION_DATABASE_URL`: cadena de conexion PostgreSQL usada por `scripts/migrate.sh`. Si no se define en la app, hace fallback a `DATABASE_URL`
- `JWT_SECRET`: secreto para firmar tokens

## Base de datos

El backend ejecuta `CREATE EXTENSION IF NOT EXISTS pgcrypto` al arrancar, porque usa `crypt(...)` para el hash y la validacion de passwords.

Tabla minima esperada:

```sql
CREATE TABLE IF NOT EXISTS users (
  id SERIAL PRIMARY KEY,
  email TEXT UNIQUE NOT NULL,
  password TEXT NOT NULL
);
```

Si `DATABASE_URL` o `MIGRATION_DATABASE_URL` no incluyen `sslmode`, la aplicacion y el flujo de migraciones anaden `sslmode=require` automaticamente.

## Desarrollo local

### 1. Instalar dependencias con Conan

```bash
conan profile detect --force
conan install . \
  --output-folder=conan/generated \
  --build=missing \
  -s build_type=Release \
  -s compiler.cppstd=20 \
  -o "libpq/*:with_openssl=True"
```

### 2. Configurar y compilar

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=conan/generated/build/Release/generators/conan_toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build --config Release
```

### 3. Ejecutar tests

```bash
ctest --test-dir build --output-on-failure
```

### 4. Arrancar la app localmente

```bash
export DATABASE_URL='postgres://usuario:password@host:5432/db?sslmode=require'
export MIGRATION_DATABASE_URL='postgres://usuario:password@host:5432/db?sslmode=require'
export JWT_SECRET='cambia-esto'
./build/src/backend
```

## Docker

### Build manual

```bash
docker compose build backend
```

### Arranque del servicio

```bash
docker compose up -d --build
```

### Ver estado y logs

```bash
docker compose ps
docker compose logs -f backend
```

## Comportamiento del servicio Docker

El servicio definido en [docker-compose.yml](/home/desa/Proyectos/backend/docker-compose.yml) hace esto:

- construye la imagen desde [Dockerfile](/home/desa/Proyectos/backend/Dockerfile)
- expone `443` del host contra `8443` del contenedor
- monta un volumen `backend_certs` en `/app/certs`
- reinicia con `restart: unless-stopped`

El entrypoint [docker/entrypoint.sh](/home/desa/Proyectos/backend/docker/entrypoint.sh):

- crea `/app/certs` si no existe
- genera un certificado autofirmado si HTTPS esta habilitado y faltan `cert.pem` o `key.pem`
- ejecuta `scripts/migrate.sh` si `MIGRATION_DATABASE_URL` esta definida
- arranca el backend en HTTP o HTTPS segun `HTTPS_ENABLED`

Por defecto, Compose espera estas variables en tu entorno o en un archivo `.env`:

```env
DATABASE_URL=postgres://usuario:password@host:5432/db?sslmode=require
MIGRATION_DATABASE_URL=postgres://usuario:password@host:5432/db?sslmode=require
JWT_SECRET=super-secreto
```

## Notas sobre Docker y build

La imagen de produccion compila con:

- `compiler.cppstd=20`
- `libpq/*:with_openssl=True`
- `BUILD_TESTING=OFF`

Eso evita que el contenedor de runtime dependa del directorio `tests/` y mantiene el build de produccion separado de la suite de pruebas.

## Endpoints

### `POST /api/auth/register`

Body:

```json
{
  "email": "user@example.com",
  "password": "secret"
}
```

### `POST /api/auth/login`

Body:

```json
{
  "email": "user@example.com",
  "password": "secret"
}
```

### `GET /api/auth/me`

Header:

```text
Authorization: Bearer <token>
```
