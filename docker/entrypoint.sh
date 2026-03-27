#!/usr/bin/env bash
set -euo pipefail

CERT_FILE="${HTTPS_CERT_FILE:-/app/certs/cert.pem}"
KEY_FILE="${HTTPS_KEY_FILE:-/app/certs/key.pem}"
CERT_DIR="$(dirname "$CERT_FILE")"
KEY_DIR="$(dirname "$KEY_FILE")"

mkdir -p "$CERT_DIR" "$KEY_DIR"

if [[ ! -f "$CERT_FILE" || ! -f "$KEY_FILE" ]]; then
  echo "[entrypoint] Generating self-signed TLS certificate for localhost..."
  openssl req -x509 -newkey rsa:2048 -nodes \
    -keyout "$KEY_FILE" \
    -out "$CERT_FILE" \
    -days 365 \
    -subj "/CN=localhost"
fi

echo "[entrypoint] Starting backend with HTTPS_ENABLED=${HTTPS_ENABLED:-true} on port ${HTTPS_PORT:-8443}"
exec /app/backend
