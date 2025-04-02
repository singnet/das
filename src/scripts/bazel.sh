#!/bin/bash

set -exou pipefail

NAME=$2
# rep;ace special chars from name
NAME=${NAME//[^[:alnum:]]/}
IMAGE_NAME="das-builder"
CONTAINER_NAME=${IMAGE_NAME}-container

ENV_VARS=$(test -f .env && echo "--env-file=.env" || echo "")

# Ensure an entrypoint is provided
if [ -z "$1" ]; then
    echo "Usage: $0 <entrypoint>"
    exit 1
fi

ENTRYPOINT="$1"
shift

# local paths
LOCAL_WORKDIR=$(pwd)
LOCAL_BIN_DIR=$LOCAL_WORKDIR/src/bin
LOCAL_CACHE="$HOME/.cache/das"

mkdir -p "$LOCAL_BIN_DIR" "$LOCAL_CACHE" 

# container paths
CONTAINER_WORKDIR=/opt/das
CONTAINER_WORKSPACE_DIR=/opt/das/src
CONTAINER_BIN_DIR=$CONTAINER_WORKSPACE_DIR/bin
CONTAINER_CACHE=/home/"${USER}"/.cache

# if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
#   echo "Removing existing container: ${CONTAINER_NAME}"
#   docker rm -f "${CONTAINER_NAME}"
# fi

docker run --rm \
  --user="$(id -u)":"$(id -g)" \
  -e BIN_DIR=$CONTAINER_BIN_DIR \
  --name="${CONTAINER_NAME}" \
  $ENV_VARS \
  --network=host \
  --volume /etc/passwd:/etc/passwd:ro \
  --volume "$LOCAL_CACHE":"$CONTAINER_CACHE" \
  --volume "$LOCAL_WORKDIR":"$CONTAINER_WORKDIR" \
  --workdir "$CONTAINER_WORKSPACE_DIR" \
  --entrypoint "${ENTRYPOINT}" \
  "${IMAGE_NAME}" \
  "${@}"
