#!/usr/bin/env bash
set -euo pipefail

is_truthy() {
  local value="${1:-}"
  value="$(printf '%s' "$value" | tr '[:upper:]' '[:lower:]')"
  [[ "$value" == "1" || "$value" == "true" || "$value" == "yes" || "$value" == "on" ]]
}

if is_truthy "${HTTPS_ENABLED:-}"; then
  CERT_FILE="${HTTPS_CERT_FILE:-/app/certs/cert.pem}"
  KEY_FILE="${HTTPS_KEY_FILE:-/app/certs/key.pem}"
  CERT_DIR="$(dirname "$CERT_FILE")"
  KEY_DIR="$(dirname "$KEY_FILE")"

  mkdir -p "$CERT_DIR" "$KEY_DIR"

  if [[ ! -f "$CERT_FILE" || ! -f "$KEY_FILE" ]]; then
    echo "[entrypoint] Generating self-signed TLS certificate for localhost..."
    openssl req -x509 -newkey rsa:2048 -nodes       -keyout "$KEY_FILE"       -out "$CERT_FILE"       -days 365       -subj "/CN=localhost"
  fi
fi

if [[ -n "${MIGRATION_DATABASE_URL:-}" ]]; then
  echo "[entrypoint] Running database migrations..."
  (cd /app && /app/migrate.sh)
fi

ACTIVE_PORT="${PORT:-8080}"
if is_truthy "${HTTPS_ENABLED:-}"; then
  ACTIVE_PORT="${HTTPS_PORT:-8443}"
fi

echo "[entrypoint] Starting backend with HTTPS_ENABLED=${HTTPS_ENABLED:-false} on port ${ACTIVE_PORT}"
exec /app/backend
