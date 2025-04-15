#!/bin/bash

IMAGE_NAME="das-builder"
CONTAINER_NAME="das-builder-bash-$(uuidgen | cut -d '-' -f 1)-$(date +%Y%m%d%H%M%S)"

ENV_VARS=$(test -f .env && echo "--env-file=.env" || echo "")

PARAMS="bash"

if [ $# -gt 0 ]; then
    PARAMS=$@
fi

docker run --rm \
    --net="host" \
    --name=$CONTAINER_NAME \
    $ENV_VARS \
    --volume /tmp:/tmp \
    --volume .:/opt/das \
    -it "${IMAGE_NAME}" \
    $PARAMS


sleep 1

if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
  echo "Removing existing container: ${CONTAINER_NAME}"
  _=$(docker rm -f "${CONTAINER_NAME}" 2>&1 > /dev/null || true)
fi

