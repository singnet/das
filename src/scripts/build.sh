#!/bin/bash

set -eoux pipefail 

detect_host_arch() {
  case "$(uname -m)" in
    x86_64)
      echo "amd64"
      ;;
    aarch64 | arm64)
      echo "arm64"
      ;;
    *)
      uname -m
      ;;
  esac
}

HOST_ARCH=$(detect_host_arch)

ARCHS="${1:-$HOST_ARCH}"
shift || true

IFS=',' read -r -a TARGET_ARCHS_ARRAY <<< "$ARCHS"

for TARGET_ARCH in "${TARGET_ARCHS_ARRAY[@]}"; do
  IMAGE_NAME=$([ "$HOST_ARCH" != "arm64" ] && [ "$HOST_ARCH" != "amd64" ] && echo "das-builder:$TARGET_ARCH" || echo "das-builder:latest")

  CONTAINER_USER=$([ "$HOST_ARCH" != "arm64" ] && [ "$HOST_ARCH" != "amd64" ] && echo "$USER" || echo "builder")

  TARGET_PLATFORM="linux/$TARGET_ARCH"
  CONTAINER_NAME="das-builder-${TARGET_ARCH}-$(uuidgen | cut -d '-' -f 1)-$(date +%Y%m%d%H%M%S)"

  ENV_VARS=$(test -f .env && echo "--env-file=.env" || echo "")

  # local paths
  LOCAL_WORKDIR=$(pwd)
  LOCAL_BIN_DIR=$LOCAL_WORKDIR/bin/$TARGET_ARCH
  LOCAL_LIB_DIR=$LOCAL_WORKDIR/lib/$TARGET_ARCH
  LOCAL_CACHE="$HOME/.cache/das/$TARGET_ARCH"

  mkdir -p $LOCAL_BIN_DIR $LOCAL_LIB_DIR $LOCAL_CACHE

  # container paths
  CONTAINER_WORKDIR=/opt/das
  CONTAINER_WORKSPACE_DIR=/opt/das/src
  CONTAINER_BIN_DIR=$CONTAINER_WORKDIR/bin/$TARGET_ARCH
  CONTAINER_LIB_DIR=$CONTAINER_WORKDIR/lib/$TARGET_ARCH
  CONTAINER_CACHE=/home/${CONTAINER_USER}/.cache

  docker run --rm \
    $([ "$HOST_ARCH" = "arm64" ] || [ "$HOST_ARCH" = "amd64" ] \
      && echo "--user=$CONTAINER_USER" \
      || echo "--user=$(id -u):$(id -g) --volume /etc/passwd:/etc/passwd:ro") \
    --privileged \
    --platform $TARGET_PLATFORM \
    --name=$CONTAINER_NAME \
    -e BIN_DIR=$CONTAINER_BIN_DIR \
    -e LIB_DIR=$CONTAINER_LIB_DIR \
    $ENV_VARS \
    --network host \
    --volume $LOCAL_CACHE:$CONTAINER_CACHE \
    --volume $LOCAL_WORKDIR:$CONTAINER_WORKDIR \
    --workdir $CONTAINER_WORKSPACE_DIR \
    ${IMAGE_NAME} \
    ./scripts/bazel_build.sh "$@"

  sleep 1

  if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "Removing existing container: ${CONTAINER_NAME}"
    _=$(docker rm -f "${CONTAINER_NAME}" 2>&1 > /dev/null || true)
  fi
done
