#!/usr/bin/env bash
set -euo pipefail

DAS_MONGODB_HOSTNAME=0.0.0.0
DAS_MONGODB_PORT=40021
DAS_MONGODB_USERNAME=admin
DAS_MONGODB_PASSWORD=admin

DAS_MORK_HOSTNAME=0.0.0.0
DAS_MORK_PORT=40022

MONGODB_IMAGE=mongodb/mongodb-community-server:8.2-ubuntu2204
MORK_IMAGE=trueagi/das:mork-server-1.1.0
POSTGRES_IMAGE=postgres:15

ARCH=$(uname -m)
case "$ARCH" in
  x86_64)
    ARCH="amd64"
    ;;
  aarch64|arm64)
    ARCH="arm64"
    ;;
  *)
    echo "Unsupported architecture: $ARCH"
    exit 1
    ;;
esac

container_running() {
  for container in "$@"; do
    if ! docker ps --format '{{.Names}}' | grep -q "^${container}$"; then
      return 1
    fi
  done
  return 0
}

start_mork_mongo() {
    #MORK
    echo "[SETUP-INFO] Starting Mork..."

    docker rm -f db-mork-regression-test 2>/dev/null || true

    docker run -d \
        --name="db-mork-regression-test" \
        --network host \
        -e MORK_SERVER_ADDR=0.0.0.0 \
        -e MORK_SERVER_PORT=40022 \
        "${MORK_IMAGE}" "$@"

    until curl --silent http://localhost:${DAS_MORK_PORT}/status/-; do
    echo "[SETUP-INFO] Waiting for MORK..."
    sleep 1
    done

    #MongoDB
    echo "[SETUP-INFO] Starting MongoDB..."

    docker rm -f db-mongo-regression-test 2>/dev/null || true

    docker run -d \
    --name db-mongo-regression-test \
    --network host \
    -e MONGO_INITDB_ROOT_USERNAME=${DAS_MONGODB_USERNAME} \
    -e MONGO_INITDB_ROOT_PASSWORD=${DAS_MONGODB_PASSWORD} \
    ${MONGODB_IMAGE} \
    mongod --port ${DAS_MONGODB_PORT} --bind_ip_all

    #WAIT DBs READY
    echo "[SETUP-INFO] Waiting DBs..."
    sleep 5
}

start_postgres_server(){
    echo "[SETUP-INFO] Starting Postgres..."

    docker rm -f postgres-regression-test 2>/dev/null || true

    docker run -d \
    --name postgres-regression-test \
    -e POSTGRES_PASSWORD=test \
    -e POSTGRES_DB=regression_test \
    -p 5434:5432 \
    ${POSTGRES_IMAGE}

    echo "[SETUP-INFO] Waiting for Postgres..."

    for i in {1..30}; do
    if docker exec postgres-regression-test pg_isready -U postgres -d regression_test; then
        echo "[SETUP-INFO] Postgres is ready"
        break
    fi

    echo "[SETUP-INFO] Waiting for Postgres..."
    sleep 1
    done

    echo "[SETUP-INFO] Loading Postgres schema..."

    docker exec -i postgres-regression-test psql -U postgres -d regression_test < ./src/tests/assets/postgres_schema.sql

    echo "[SETUP-INFO] Setup completed successfully."
}

start_mork_mongo
start_postgres_server

echo "[SETUP-INFO] Building project..."

make build-all

echo "[SETUP-INFO] Running regression tests..."

./src/scripts/bazel.sh run //tests/regression:adapterdb_main /opt/das/src/tests/assets/adapterdb_config.json