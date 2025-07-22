#!/bin/bash

set -eoux pipefail

IMAGE_NAME="trueagi/das:mork-server-0.10.2"
CONTAINER_NAME="das-mork-server"

docker run --rm \
    --name="${CONTAINER_NAME}" \
    --network host \
    --privileged \
    "${IMAGE_NAME}" "$@"
