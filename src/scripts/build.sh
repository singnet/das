#!/bin/bash

set -eoux pipefail

IMAGE_NAME="das-builder"
CONTAINER_NAME=${IMAGE_NAME}-container

ENV_VARS=$(test -f .env && echo "--env-file=.env" || echo "")

# local paths
LOCAL_WORKDIR=$(pwd)
LOCAL_BIN_DIR=$LOCAL_WORKDIR/src/bin
LOCAL_CACHE="$HOME/.cache/das"

mkdir -p $LOCAL_BIN_DIR $LOCAL_CACHE

# container paths
CONTAINER_WORKDIR=/opt/das
CONTAINER_WORKSPACE_DIR=/opt/das/src
CONTAINER_BIN_DIR=$CONTAINER_WORKSPACE_DIR/bin
CONTAINER_CACHE=/home/${USER}/.cache

if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
  echo "Removing existing container: ${CONTAINER_NAME}"
  docker rm -f "${CONTAINER_NAME}"
fi

docker run --rm \
  --user=$(id -u):$(id -g) \
  --name=$CONTAINER_NAME \
  -e BIN_DIR=$CONTAINER_BIN_DIR \
  ${ENV_VARS} \
  --network host \
  --volume /etc/passwd:/etc/passwd:ro \
  --volume $LOCAL_CACHE:$CONTAINER_CACHE \
  --volume $LOCAL_WORKDIR:$CONTAINER_WORKDIR \
  --workdir $CONTAINER_WORKSPACE_DIR \
  ${IMAGE_NAME} \
  ./scripts/bazel_build.sh

