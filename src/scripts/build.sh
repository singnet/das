#!/bin/bash

IMAGE_NAME="das-attention-broker-builder"
CONTAINER_NAME=${IMAGE_NAME}-container

docker run --rm \
    -e _USER=$(id -u) \
    -e _GROUP=$(id -g) \
    --name=$CONTAINER_NAME \
    --volume .:/opt/das-attention-broker \
    --workdir /opt/das-attention-broker \
    ${IMAGE_NAME} \
    ./scripts/bazel_build.sh

