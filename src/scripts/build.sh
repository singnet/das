#!/bin/bash

OUTPUT_DIR_BINARIES="bin"
OUTPUT_DIR_LIBRARIES="lib"
DAS_LIBRARY_NAME="hyperon_das"

set -eoux pipefail

ARCH=$(uname -m)

IMAGE_NAME="das-builder"
CONTAINER_NAME="das-builder-$(uuidgen | cut -d '-' -f 1)-$(date +%Y%m%d%H%M%S)"
CONTAINER_USER=$([ "$ARCH" != "arm64" ] && echo "$USER" || echo "builder")

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
CONTAINER_CACHE=/home/${CONTAINER_USER}/.cache

docker run --rm \
  $([ "$ARCH" != "arm64" ] && echo "--user=$(id -u):$(id -g) --volume /etc/passwd:/etc/passwd:ro" || echo "--user=$CONTAINER_USER") \
  --privileged \
  --name=$CONTAINER_NAME \
  -e BIN_DIR=$CONTAINER_BIN_DIR \
  $ENV_VARS \
  --network host \
  --volume $LOCAL_CACHE:$CONTAINER_CACHE \
  --volume $LOCAL_WORKDIR:$CONTAINER_WORKDIR \
  --workdir $CONTAINER_WORKSPACE_DIR \
  ${IMAGE_NAME} \
  ./scripts/bazel_build.sh

sleep 1

if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
  echo "Removing existing container: ${CONTAINER_NAME}"
  _=$(docker rm -f "${CONTAINER_NAME}" 2>&1 > /dev/null || true)
fi

mkdir -p $OUTPUT_DIR_BINARIES
mkdir -p $OUTPUT_DIR_LIBRARIES
cp -f src/bin/* $OUTPUT_DIR_BINARIES
mv -f $OUTPUT_DIR_BINARIES/das.so $OUTPUT_DIR_LIBRARIES/$DAS_LIBRARY_NAME.so
