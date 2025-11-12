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
    --env MORK_SERVER_ADDR=${DAS_MORK_HOSTNAME:-localhost} \
    --env MORK_SERVER_PORT=${DAS_MORK_PORT:-8000} \
    --volume "${FILE}":/app/file.metta \
    --volume `pwd`/src/scripts/mork_loader.py:/app/mork_loader.py \
    --workdir /app \
    "${IMAGE_NAME}" \
    --file /app/file.metta
