#!/bin/bash

IMAGE_NAME="das-attention-broker-builder"
CONTAINER_NAME=${IMAGE_NAME}-container
LOCAL_CACHE=/tmp/bazel_cache
mkdir -p $LOCAL_CACHE

docker run --rm \
    -e _USER=$(id -u) \
    -e _GROUP=$(id -g) \
    --network=host \
    --name=$CONTAINER_NAME \
    --volume .:/opt/das-attention-broker \
    --volume $LOCAL_CACHE:/root/.cache/bazel \
    --workdir /opt/das-attention-broker/src \
    ${IMAGE_NAME} \
    ../scripts/bazel_test.sh

