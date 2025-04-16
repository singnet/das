#!/bin/bash

CONTAINER_NAME="$1"
shift

ENV_VARS=$(test -f .env && echo "--env-file=.env" || echo "")

if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
  echo "Removing existing container: ${CONTAINER_NAME}"
  docker rm -f "${CONTAINER_NAME}"
fi

mkdir -p bin
docker run \
    --name="$CONTAINER_NAME" \
    --network host \
    --volume .:/opt/das \
    --workdir /opt/das \
    $ENV_VARS \
    das-builder \
    "src/bin/$CONTAINER_NAME" "$@"

# sleep 1
# docker rm -f "$CONTAINER_NAME"
