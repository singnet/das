#!/bin/bash

set -eoux pipefail

IMAGE_NAME="trueagi/das:mork-server-1.0.0"
CONTAINER_NAME="das-mork-server-$(date +%Y%m%d%H%M%S)"

docker run --rm \
    --name="${CONTAINER_NAME}" \
    --network host \
    -e MORK_SERVER_ADDR=0.0.0.0 \
    -e MORK_SERVER_PORT=${DAS_MORK_PORT:-40022} \
    "${IMAGE_NAME}" "$@"

sleep 1

if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
  echo "Removing existing container: ${CONTAINER_NAME}"
  _=$(docker rm -f "${CONTAINER_NAME}" 2>&1 > /dev/null || true)
fi
