#!/bin/bash

set -eoux pipefail

IMAGE_NAME="marcocapozzoli/das-mork"
CONTAINER_NAME="das-mork-server"

docker run --rm \
    --name="${CONTAINER_NAME}" \
    --network host \
    "${IMAGE_NAME}" "$@"
