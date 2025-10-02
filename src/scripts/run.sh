#!/bin/bash

set -eoux pipefail

BINARY_NAME="${1}"
shift
# Optional: Maximum memory allocation for the Docker container (default: half of system memory)
HALF_SYSTEM_MEMORY=$(free -m | awk '/^Mem:/{print $2*0.7 "m"}')
MAX_MEMORY=${MAX_MEMORY:-"$HALF_SYSTEM_MEMORY"}
IMAGE_NAME="das-builder"
CONTAINER_NAME="das-${BINARY_NAME}-$(uuidgen | cut -d '-' -f 1)-$(date +%Y%m%d%H%M%S)"

ENV_VARS=$(test -f .env && echo "--env-file=.env" || echo "")

mkdir -p bin
docker run --rm \
    --name="${CONTAINER_NAME}" \
    --memory="${MAX_MEMORY}" \
    --network host \
    --volume .:/opt/das \
    --workdir /opt/das \
    $ENV_VARS \
    "${IMAGE_NAME}" \
    "bin/${BINARY_NAME}" "$@"

sleep 1

if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
  echo "Removing existing container: ${CONTAINER_NAME}"
  _=$(docker rm -f "${CONTAINER_NAME}" 2>&1 > /dev/null || true)
fi
