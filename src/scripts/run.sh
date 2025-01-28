#!/bin/bash

CONTAINER_NAME="$1"

ENV_VARS=$(printenv | awk -F= '{print "--env "$1}')

mkdir -p bin
docker run \
    --name="$CONTAINER_NAME" \
    --network host \
    --volume .:/opt/das-attention-broker \
    --workdir /opt/das-attention-broker \
    $ENV_VARS \
    das-attention-broker-builder \
    ./bin/"$1" "$2"

sleep 1
docker rm "$CONTAINER_NAME"
