#!/bin/bash

IMAGE_NAME="das-attention-broker-builder"
CONTAINER_NAME=${IMAGE_NAME}-container

# local paths
LOCAL_WORKDIR=$(pwd)
LOCAL_BIN_DIR=$LOCAL_WORKDIR/bin
LOCAL_BAZEL_CACHE=$LOCAL_WORKDIR/docker/volumes/cache/bazel
LOCAL_BAZELISK_CACHE=$LOCAL_WORKDIR/docker/volumes/cache/bazelisk
mkdir -p $LOCAL_BAZEL_CACHE $LOCAL_BAZELISK_CACHE $LOCAL_BIN_DIR

# container paths
CONTAINER_WORKDIR=/opt/das-attention-broker
CONTAINER_BIN_DIR=$CONTAINER_WORKDIR/bin
CONTAINER_BAZEL_CACHE=/home/${USER}/.cache/bazel
CONTAINER_BAZELISK_CACHE=/home/${USER}/.cache/bazelisk
CONTAINER_WORKSPACE_DIR_CPP=$CONTAINER_WORKDIR/cpp
CONTAINER_WORKSPACE_DIR_PYTHON=$CONTAINER_WORKDIR/python


docker run --rm \
    --user=$(id -u):$(id -g) \
    --name=$CONTAINER_NAME \
    -e BIN_DIR=$CONTAINER_BIN_DIR \
    -e WORKSPACE_DIR=$CONTAINER_WORKDIR \
    -e CPP_WORKSPACE_DIR=$CONTAINER_WORKSPACE_DIR_CPP \
    -e PYTHON_WORKSPACE_DIR=$CONTAINER_WORKSPACE_DIR_PYTHON \
    --volume /etc/passwd:/etc/passwd:ro \
    --volume $LOCAL_BAZEL_CACHE:$CONTAINER_BAZEL_CACHE \
    --volume $LOCAL_BAZELISK_CACHE:$CONTAINER_BAZELISK_CACHE \
    --volume $LOCAL_WORKDIR:$CONTAINER_WORKDIR \
    --workdir $CONTAINER_WORKDIR \
    ${IMAGE_NAME} \
    ./scripts/bazel_build.sh

