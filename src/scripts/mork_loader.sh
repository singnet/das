#!/bin/bash

set -e

if [ -z "$1" ]; then
    echo "Usage: mork_loader.sh FILE"
    exit 1
else
    FILE=$1
fi

IMAGE_NAME="das-mork-loader"

CONTAINER_NAME="${IMAGE_NAME}-$(uuidgen | cut -d '-' -f 1)-$(date +%Y%m%d%H%M%S)"

docker run --rm \
    --name="${CONTAINER_NAME}" \
    --network host \
    --volume "${FILE}":/app/file.metta \
    --workdir /app \
    "${IMAGE_NAME}" \
    --file /app/file.metta