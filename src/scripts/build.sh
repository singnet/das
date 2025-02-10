#!/bin/bash

set -eoux pipefail

IMAGE_NAME="das-builder"
CONTAINER_NAME=${IMAGE_NAME}-container

# local paths
LOCAL_WORKDIR=$(pwd)
LOCAL_BIN_DIR=$LOCAL_WORKDIR/bin
LOCAL_BAZEL_CACHE="$HOME/.cache/das/bazel"
LOCAL_BAZELISK_CACHE="$HOME/.cache/das/bazelisk"
LOCAL_PIPTOOLS_CACHE="$HOME/.cache/das/pip-tools"
LOCAL_PIP_CACHE="$HOME/.cache/das/pip"
mkdir -p \
    $LOCAL_BAZEL_CACHE \
    $LOCAL_BAZELISK_CACHE \
    $LOCAL_BIN_DIR \
    $LOCAL_PIPTOOLS_CACHE \
    $LOCAL_PIP_CACHE

# container paths
CONTAINER_WORKDIR=/opt/das
CONTAINER_BIN_DIR=$CONTAINER_WORKDIR/bin
CONTAINER_BAZEL_CACHE=/home/${USER}/.cache/bazel
CONTAINER_PIP_CACHE=/home/${USER}/.cache/pip
CONTAINER_PIPTOOLS_CACHE=/home/${USER}/.cache/pip-tools
CONTAINER_BAZELISK_CACHE=/home/${USER}/.cache/bazelisk
CONTAINER_WORKSPACE_DIR_CPP=$CONTAINER_WORKDIR/cpp
CONTAINER_WORKSPACE_DIR_PYTHON=$CONTAINER_WORKDIR/python


docker run --rm \
    --user="$(id -u):$(id -g)" \
    --name=$CONTAINER_NAME \
    -e BIN_DIR=$CONTAINER_BIN_DIR \
    -e CPP_WORKSPACE_DIR=$CONTAINER_WORKSPACE_DIR_CPP \
    -e PYTHON_WORKSPACE_DIR=$CONTAINER_WORKSPACE_DIR_PYTHON \
    --volume /etc/passwd:/etc/passwd:ro \
    --volume "$LOCAL_PIP_CACHE:$CONTAINER_PIP_CACHE" \
    --volume "$LOCAL_PIPTOOLS_CACHE:$CONTAINER_PIPTOOLS_CACHE" \
    --volume "$LOCAL_BAZEL_CACHE:$CONTAINER_BAZEL_CACHE" \
    --volume "$LOCAL_BAZELISK_CACHE:$CONTAINER_BAZELISK_CACHE" \
    --volume "$LOCAL_WORKDIR:$CONTAINER_WORKDIR" \
    --workdir "$CONTAINER_WORKDIR" \
    ${IMAGE_NAME} \
    ./scripts/bazel_build.sh

