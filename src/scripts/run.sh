#!/bin/bash

CONTAINER_NAME="$1"
shift

ENV_VARS=$(printenv | awk -F= '{print "--env "$1}')

mkdir -p bin
docker run \
    --name="$CONTAINER_NAME" \
    --network host \
    --volume .:/opt/das \
    --workdir /opt/das \
    $ENV_VARS \
    das-builder \
    "src/bin/$CONTAINER_NAME" "$@"

sleep 1
docker rm "$CONTAINER_NAME"
