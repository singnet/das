#!/bin/bash

sufix="-word_query_evolution"

MY_ADDRESS="localhost:24003"
QA_ADDRESS="localhost:40002"
MY_PORT_RANGE="52000:52999"
QUERY_WORD_1="aaa"
QUERY_WORD_2="bbb"

if [[ -z "$1" ]]; then
    RENT_RATE=0.25
else
    RENT_RATE=$1
fi
if [[ -z "$2" ]]; then
    SPREADING_RATE_LOWERBOUND=0.10
else
    SPREADING_RATE_LOWERBOUND=$2
fi
if [[ -z "$3" ]]; then
    SPREADING_RATE_UPPERBOUND=0.10
else
    SPREADING_RATE_UPPERBOUND=$3
fi
if [[ -z "$4" ]]; then
    ELITISM_RATE=0.08
else
    ELITISM_RATE=$4
fi
if [[ -z "$5" ]]; then
    echo "Error: no context was defined"
    exit 1
else
    CONTEXT=$5
fi

set -eou pipefail

IMAGE_NAME="das-builder"
CONTAINER_NAME=${IMAGE_NAME}-container$sufix
BAZEL_CMD="/opt/bazel/bazelisk"

# local paths
LOCAL_WORKDIR=$(pwd)
LOCAL_BIN_DIR=$LOCAL_WORKDIR/src/bin
LOCAL_ASPECT_CACHE="$HOME/.cache/das/aspect"
LOCAL_BAZEL_CACHE="$HOME/.cache/das/bazel"
LOCAL_BAZELISK_CACHE="$HOME/.cache/das/bazelisk"
LOCAL_PIPTOOLS_CACHE="$HOME/.cache/das/pip-tools"
LOCAL_PIP_CACHE="$HOME/.cache/das/pip"

mkdir -p \
  "$LOCAL_ASPECT_CACHE" \
  "$LOCAL_BIN_DIR" \
  "$LOCAL_BAZEL_CACHE" \
  "$LOCAL_BAZELISK_CACHE" \
  "$LOCAL_PIPTOOLS_CACHE" \
  "$LOCAL_PIP_CACHE"

# container paths
CONTAINER_WORKDIR=/opt/das
CONTAINER_WORKSPACE_DIR=/opt/das/src
CONTAINER_BIN_DIR=$CONTAINER_WORKSPACE_DIR/bin
CONTAINER_ASPECT_CACHE=/root/.cache/aspect
CONTAINER_BAZEL_CACHE=/root/.cache/bazel
CONTAINER_PIP_CACHE=/root/.cache/pip
CONTAINER_PIPTOOLS_CACHE=/root/.cache/pip-tools
CONTAINER_BAZELISK_CACHE=/root/.cache/bazelisk

if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
  echo "Removing existing container: ${CONTAINER_NAME}"
  docker rm -f "${CONTAINER_NAME}"
fi

docker run --rm \
  -ti \
  --rm \
  --name=$CONTAINER_NAME \
  -e BIN_DIR=$CONTAINER_BIN_DIR \
  --network=host \
  --env DAS_REDIS_HOSTNAME=localhost \
  --env DAS_REDIS_PORT=40020 \
  --env DAS_USE_REDIS_CLUSTER=False \
  --env DAS_MONGODB_HOSTNAME=localhost \
  --env DAS_MONGODB_PORT=40021 \
  --env DAS_MONGODB_USERNAME=admin \
  --env DAS_MONGODB_PASSWORD=admin \
  --env DAS_MONGODB_CHUNK_SIZE=1000000 \
  --volume "$LOCAL_PIP_CACHE":"$CONTAINER_PIP_CACHE" \
  --volume "$LOCAL_PIPTOOLS_CACHE":"$CONTAINER_PIPTOOLS_CACHE" \
  --volume "$LOCAL_ASPECT_CACHE":"$CONTAINER_ASPECT_CACHE" \
  --volume "$LOCAL_BAZEL_CACHE":"$CONTAINER_BAZEL_CACHE" \
  --volume "$LOCAL_BAZELISK_CACHE":"$CONTAINER_BAZELISK_CACHE" \
  --volume "$LOCAL_WORKDIR":"$CONTAINER_WORKDIR" \
  --workdir "$CONTAINER_WORKSPACE_DIR" \
  "${IMAGE_NAME}" \
  /opt/das/bin/word_query_evolution ${MY_ADDRESS} ${QA_ADDRESS} ${MY_PORT_RANGE} ${CONTEXT} ${QUERY_WORD_1} ${QUERY_WORD_2} ${RENT_RATE} ${SPREADING_RATE_LOWERBOUND} ${SPREADING_RATE_UPPERBOUND} ${ELITISM_RATE}
