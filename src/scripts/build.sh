#!/bin/bash

IMAGE_NAME="das-attention-broker-builder"
CONTAINER_NAME=${IMAGE_NAME}-container
BAZEL_CACHE=./docker/volumes/cache/bazel
BAZELISK_CACHE=./docker/volumes/cache/bazelisk
mkdir -p $BAZEL_CACHE $BAZELISK_CACHE

docker run --rm \
    -e _USER=$(id -u) \
    -e _GROUP=$(id -g) \
    --name=$CONTAINER_NAME \
    --volume $BAZEL_CACHE:/root/.cache/bazel \
    --volume $BAZELISK_CACHE:/root/.cache/bazelisk \
    --volume .:/opt/das-attention-broker \
    --workdir /opt/das-attention-broker \
    ${IMAGE_NAME} \
    ./scripts/bazel_build.sh

