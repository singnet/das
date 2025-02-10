#!/bin/bash

IMAGE_NAME="das-builder"
CONTAINER_NAME=${IMAGE_NAME}-container

# local paths
LOCAL_WORKDIR=$(pwd)
LOCAL_BAZEL_CACHE="$HOME/.cache/das/bazel"
LOCAL_BAZELISK_CACHE="$HOME/.cache/das/bazelisk"
mkdir -p $LOCAL_BAZEL_CACHE $LOCAL_BAZELISK_CACHE

# container paths
CONTAINER_WORKDIR=/opt/das
CONTAINER_BAZEL_CACHE=/home/${USER}/.cache/bazel
CONTAINER_BAZELISK_CACHE=/home/${USER}/.cache/bazelisk
CONTAINER_WORKSPACE_DIR=$CONTAINER_WORKDIR/cpp

docker run --rm \
    --user=$(id -u):$(id -g) \
    --network=host \
    --name=$CONTAINER_NAME \
    -e WORKSPACE_DIR=$CONTAINER_WORKSPACE_DIR \
    --volume /etc/passwd:/etc/passwd:ro \
    --volume $LOCAL_BAZEL_CACHE:$CONTAINER_BAZEL_CACHE \
    --volume $LOCAL_BAZELISK_CACHE:$CONTAINER_BAZELISK_CACHE \
    --volume $LOCAL_WORKDIR:$CONTAINER_WORKDIR \
    --workdir $CONTAINER_WORKDIR \
    ${IMAGE_NAME} \
    ./scripts/bazel_test.sh

