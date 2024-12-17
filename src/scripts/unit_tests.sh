#!/bin/bash

CONTAINER_NAME="das-attention-broker-build"

mkdir -p bin
docker run \
    --name=$CONTAINER_NAME \
    --volume .:/opt/das-attention-broker \
    --workdir /opt/das-attention-broker/src \
    das-attention-broker-builder \
    ../scripts/bazel_test.sh

sleep 1
docker rm $CONTAINER_NAME
