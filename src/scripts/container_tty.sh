#!/bin/bash

CONTAINER_NAME="das-builder-bash"

PARAMS="bash"

if [ $# -gt 0 ]; then
    PARAMS=$@
fi

if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
  echo "Removing existing container: ${CONTAINER_NAME}"
  docker rm -f "${CONTAINER_NAME}"
fi

ENV_VARS=$(printenv | awk -F= '{print "--env "$1}')

docker run --rm \
    --net="host" \
    --volume /etc/passwd:/etc/passwd:ro \
    $ENV_VARS \
    --ulimit nofile=1024:4096 \
    --name=$CONTAINER_NAME \
    --volume /tmp:/tmp \
    --volume .:/opt/das \
    -it das-builder \
    $PARAMS

sleep 1