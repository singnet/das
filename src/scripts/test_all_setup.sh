#!/usr/bin/env bash
set -euo pipefail

# VARIABLES

DAS_REDIS_HOSTNAME=0.0.0.0
DAS_REDIS_PORT=40020

DAS_MONGODB_HOSTNAME=0.0.0.0
DAS_MONGODB_PORT=40021
DAS_MONGODB_USERNAME=admin
DAS_MONGODB_PASSWORD=admin

DAS_MORK_HOSTNAME=0.0.0.0
DAS_MORK_PORT=40022

REDIS_IMAGE=redis:7.2.3-alpine
MONGODB_IMAGE=mongodb/mongodb-community-server:8.2-ubuntu2204
METTA_LOADER_IMAGE=trueagi/das:1.0.0-metta-parser
MORK_IMAGE=trueagi/das:mork-server-1.0.4
MORK_LOADER_IMAGE=trueagi/das:mork-loader-1.0.4
MORK_SERVER_IMAGE=trueagi/das:mork-server-1.0.4
POSTGRES_IMAGE=postgres:15

ANIMALS_FILE=/tmp/animals.metta

ATTENTION_BROKER_SERVICE_PORT=40001

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

# FUNCTIONS

container_running() {
  for container in "$@"; do
    if ! docker ps --format '{{.Names}}' | grep -q "^${container}$"; then
      return 1
    fi
  done
  return 0
}

start_redis_mongo() {
    #REDIS
    echo "[SETUP-INFO] Starting Redis..."

    docker rm -f db-redis-test-container 2>/dev/null || true

    docker run -d \
    --name db-redis-test-container \
    --network host \
    -e DAS_REDIS_HOSTNAME=${DAS_REDIS_HOSTNAME} \
    ${REDIS_IMAGE} \
    redis-server \
    --port ${DAS_REDIS_PORT} \
    --appendonly no \
    --save "" \
    --protected-mode no

    started=true

    #MongoDB
    echo "[SETUP-INFO] Starting MongoDB..."

    docker rm -f db-mongo-test-container 2>/dev/null || true

    docker run -d \
    --name db-mongo-test-container \
    --network host \
    -e MONGO_INITDB_ROOT_USERNAME=${DAS_MONGODB_USERNAME} \
    -e MONGO_INITDB_ROOT_PASSWORD=${DAS_MONGODB_PASSWORD} \
    ${MONGODB_IMAGE} \
    mongod --port ${DAS_MONGODB_PORT} --bind_ip_all

    #WAIT DBs READY
    echo "[SETUP-INFO] Waiting DBs..."
    sleep 10

    #LOAD KNOWLEDGE BASE
    echo "[SETUP-INFO] Loading knowledge base with Metta Parser..."

    docker run --rm \
    --network host \
    -v ${ANIMALS_FILE}:${ANIMALS_FILE}:ro \
    -e DAS_REDIS_HOSTNAME=${DAS_REDIS_HOSTNAME} \
    -e DAS_REDIS_PORT=${DAS_REDIS_PORT} \
    -e DAS_MONGODB_HOSTNAME=${DAS_MONGODB_HOSTNAME} \
    -e DAS_MONGODB_PORT=${DAS_MONGODB_PORT} \
    -e DAS_MONGODB_USERNAME=${DAS_MONGODB_USERNAME} \
    -e DAS_MONGODB_PASSWORD=${DAS_MONGODB_PASSWORD} \
    ${METTA_LOADER_IMAGE} \
    db_loader ${ANIMALS_FILE}
}

start_attention_broker(){
    echo "[SETUP-INFO] Starting attention broker..."

    docker run -d \
        --name="das-attention-broker-service" \
        --network host \
        --volume .:/opt/das \
        --volume /tmp:/tmp \
        --workdir /opt/das \
        "das-builder:${ARCH}" \
        "bin/$ARCH/attention_broker_service" "0.0.0.0:40001"
}

start_mork_server() {
    echo "[SETUP-INFO] Starting MORK server..."

    docker run -d \
        --name="mork-test-server" \
        --network host \
        -e MORK_SERVER_ADDR=0.0.0.0 \
        -e MORK_SERVER_PORT=40022 \
        "${MORK_SERVER_IMAGE}" "$@"

    until curl --silent http://localhost:${DAS_MORK_PORT}/status/-; do
    echo "[SETUP-INFO] Waiting for MORK..."
    sleep 1
    done
}

start_postgres_server(){
    echo "[SETUP-INFO] Starting Postgres..."

    docker run -d \
    --name pg-test \
    -e POSTGRES_PASSWORD=test \
    -e POSTGRES_DB=postgres_wrapper_test \
    -p 5433:5432 \
    ${POSTGRES_IMAGE}

    # WAIT POSTGRES READY

    echo "[SETUP-INFO] Waiting for Postgres..."

    for i in {1..30}; do
    if docker exec pg-test pg_isready -U postgres -d postgres_wrapper_test; then
        echo "[SETUP-INFO] Postgres is ready"
        break
    fi

    echo "[SETUP-INFO] Waiting for Postgres..."
    sleep 1
    done

    # LOAD POSTGRES SCHEMA

    echo "[SETUP-INFO] Loading Postgres schema..."

    docker exec -i pg-test \
    psql -U postgres -d postgres_wrapper_test \
    < src/scripts/postgres_setup.sql

    echo "[SETUP-INFO] Setup completed successfully."
}

# SETUP STEPS

# 1. DOWNLOAD KNOWLEDGE BASE

echo "[SETUP-INFO] Downloading knowledge base..."

curl -L \
https://raw.githubusercontent.com/singnet/das-toolbox/refs/heads/master/das-cli/src/examples/data/animals.metta \
-o ${ANIMALS_FILE}

# 2. START REDIS/MONGO AND LOAD DATABASE.

if container_running db-redis-test-container db-mongo-test-container; then
    echo "[SETUP-INFO] Redis and MongoDB containers are already running. Skipping startup and data loading."
else
    start_redis_mongo
fi

# 3. START MORK SERVER

if container_running mork-test-server; then
    echo "[SETUP-INFO] Mork server container is already running. Skipping startup."
else
    start_mork_server
fi

# 4.START POSTGRES SERVER

if container_running pg-test; then
    echo "[SETUP-INFO] Postgres container is already running. Skipping startup."
else
    start_postgres_server
fi

# 5. BUILD BINARIES (Necessary for test_db_loader, will skip if they already exist)

echo "[SETUP-INFO] Building project..."

make build-all

# 6. START ATTENTION BROKER

if container_running das-attention-broker-service; then
    echo "[SETUP-INFO] Attention broker container is already running. Skipping startup."
else
    start_attention_broker
fi
