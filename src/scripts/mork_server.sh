#!/bin/bash

set -eoux pipefail

IMAGE_NAME="das-mork-server"
CONTAINER_NAME="das-mork-server-$(uuidgen | cut -d '-' -f 1)-$(date +%Y%m%d%H%M%S)"

docker run --rm \
    --name="${CONTAINER_NAME}" \
    --network host \
    "${IMAGE_NAME}" "$@"

sleep 1

if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
  echo "Removing existing container: ${CONTAINER_NAME}"
  _=$(docker rm -f "${CONTAINER_NAME}" 2>&1 > /dev/null || true)
fi
