#!/usr/bin/env bash
set -euo pipefail

if [ -z "${MIGRATION_DATABASE_URL:-}" ]; then
  echo "ERROR: MIGRATION_DATABASE_URL no está definida"
  exit 1
fi

shopt -s nullglob
migration_files=(migrations/*.sql)

if [ ${#migration_files[@]} -eq 0 ]; then
  echo "No se encontraron archivos de migración"
  exit 0
fi

psql "$MIGRATION_DATABASE_URL" -v ON_ERROR_STOP=1 -c '
CREATE TABLE IF NOT EXISTS schema_migrations (
  version VARCHAR(255) PRIMARY KEY,
  applied_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);'

for file in "${migration_files[@]}"; do
  filename="$(basename "$file")"

  already_applied=$(psql "$MIGRATION_DATABASE_URL" -tAc \
    "SELECT 1 FROM schema_migrations WHERE version = '$filename' LIMIT 1;")

  if [ "$already_applied" = "1" ]; then
    echo "Saltando $filename (ya aplicada)"
    continue
  fi

  echo "Aplicando $filename"
  psql "$MIGRATION_DATABASE_URL" -v ON_ERROR_STOP=1 -f "$file"

  psql "$MIGRATION_DATABASE_URL" -v ON_ERROR_STOP=1 -c \
    "INSERT INTO schema_migrations(version) VALUES ('$filename');"
done

echo "Migraciones completadas"
