#!/bin/bash

set -euo pipefail

IMAGE_NAME="trueagi/das:mork-server-1.1.0"
MORK_PORT="${DAS_MORK_PORT:-40022}"
CONTAINER_NAME="das-mork-server-${MORK_PORT}"

docker rm -f "${CONTAINER_NAME}" 2>/dev/null || true

docker run -d --rm \
    --name="${CONTAINER_NAME}" \
    --network host \
    -e MORK_SERVER_ADDR=0.0.0.0 \
    -e MORK_SERVER_PORT="${MORK_PORT}" \
    "${IMAGE_NAME}" "$@"
