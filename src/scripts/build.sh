#!/bin/bash

CONTAINER_NAME="das-attention-broker-build"

mkdir -p bin
docker run --rm \
    --name=$CONTAINER_NAME \
    --volume .:/opt/das-attention-broker \
    --workdir /opt/das-attention-broker \
    das-attention-broker-builder \
    ./scripts/bazel_build.sh

