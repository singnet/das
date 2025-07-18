#!/bin/bash

set -exou pipefail

ARCH=$(uname -m)

IMAGE_NAME="das-builder"
CONTAINER_USER=$([ "$ARCH" != "arm64" ] && echo "$USER" || echo "builder")
CONTAINER_NAME="das-bazel-cmd-$(uuidgen | cut -d '-' -f 1)-$(date +%Y%m%d%H%M%S)"

ENV_VARS=$(test -f .env && echo "--env-file=.env" || echo "")

# local paths
LOCAL_WORKDIR=$(pwd)
LOCAL_BIN_DIR=$LOCAL_WORKDIR/bin
LOCAL_LIB_DIR=$LOCAL_WORKDIR/lib
LOCAL_CACHE="$HOME/.cache/das"

mkdir -p "$LOCAL_LIB_DIR" "$LOCAL_BIN_DIR" "$LOCAL_CACHE"

# container paths
CONTAINER_WORKDIR=/opt/das
CONTAINER_WORKSPACE_DIR=/opt/das/src
CONTAINER_LIB_DIR=$CONTAINER_WORKDIR/lib
CONTAINER_BIN_DIR=$CONTAINER_WORKDIR/bin
CONTAINER_CACHE=/home/"${CONTAINER_USER}"/.cache

docker run --rm \
  $([ "$ARCH" != "arm64" ] && echo "--user=$(id -u):$(id -g) --volume /etc/passwd:/etc/passwd:ro" || echo "--user=$CONTAINER_USER") \
  --privileged \
  --name="${CONTAINER_NAME}" \
  -e BIN_DIR=$CONTAINER_BIN_DIR \
  -e LIB_DIR=$CONTAINER_LIB_DIR \
  --network=host \
  --volume "$LOCAL_CACHE":"$CONTAINER_CACHE" \
  --volume "$LOCAL_WORKDIR":"$CONTAINER_WORKDIR" \
  --volume "/tmp:/tmp" \
  --workdir "$CONTAINER_WORKSPACE_DIR" \
  "${IMAGE_NAME}" \
  ./scripts/bazel_exec.sh "$@"

sleep 1

if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
  echo "Removing existing container: ${CONTAINER_NAME}"
  _=$(docker rm -f "${CONTAINER_NAME}" 2>&1 > /dev/null || true)
fi
