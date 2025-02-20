#!/bin/bash

set -eou pipefail

IMAGE_NAME="das-builder"
CONTAINER_NAME=${IMAGE_NAME}-container
BAZEL_CMD="/opt/bazel/bazelisk"

# local paths
LOCAL_WORKDIR=$(pwd)
LOCAL_BIN_DIR=$LOCAL_WORKDIR/src/bin
LOCAL_CACHE="$HOME/.cache/das"

mkdir -p \
  "$LOCAL_BIN_DIR" \
  "$LOCAL_CACHE"

# container paths
CONTAINER_WORKDIR=/opt/das
CONTAINER_WORKSPACE_DIR=/opt/das/src
CONTAINER_BIN_DIR=$CONTAINER_WORKSPACE_DIR/bin
CONTAINER_CACHE="/home/$USER/.cache"

docker run --rm \
  --user="$(id -u)":"$(id -g)" \
  --name=$CONTAINER_NAME \
  -e BIN_DIR=$CONTAINER_BIN_DIR \
  --network=host \
  --volume /etc/passwd:/etc/passwd:ro \
  --volume "$LOCAL_CACHE":"$CONTAINER_CACHE" \
  --volume "$LOCAL_WORKDIR":"$CONTAINER_WORKDIR" \
  --workdir "$CONTAINER_WORKSPACE_DIR" \
  --entrypoint "$BAZEL_CMD" \
  "${IMAGE_NAME}" \
  "$@"
