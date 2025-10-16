#!/bin/bash

set -e

if [ -z "$1" ]; then
    echo "Usage: mork_loader.sh FILE"
    exit 1
else
    FILE=$1
fi

IMAGE_NAME="trueagi/das:mork-loader-0.10.2"
CONTAINER_NAME="das-mork-loader-$(date +%Y%m%d%H%M%S)"

docker run --rm \
    --name="${CONTAINER_NAME}" \
    --network host \
    --volume "${FILE}":/app/file.metta \
    --volume `pwd`/src/scripts/mork_loader.py:/app/mork_loader.py \
    --workdir /app \
    "${IMAGE_NAME}" \
    --file /app/file.metta

sleep 1

if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
  echo "Removing existing container: ${CONTAINER_NAME}"
  _=$(docker rm -f "${CONTAINER_NAME}" 2>&1 > /dev/null || true)
fi
