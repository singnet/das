#!/bin/bash
set -eoux pipefail

detect_host_arch() {
  case "$(uname -m)" in
    x86_64) echo "amd64" ;;
    aarch64 | arm64) echo "arm64" ;;
    *) uname -m ;;
  esac
}

HOST_ARCH=$(detect_host_arch)
PACKAGE_TYPE=${1:-}
shift || true

TARGET_PLATFORM="linux/$HOST_ARCH"
CONTAINER_NAME="das-builder-${HOST_ARCH}-$(uuidgen | cut -d '-' -f 1)-$(date +%Y%m%d%H%M%S)"
ENV_VARS=$(test -f .env && echo "--env-file=.env" || echo "")

# Docker user flags
if [ "$HOST_ARCH" = "amd64" ]; then
  CONTAINER_USER_NAME="$USER"
  DOCKER_USER_FLAGS="--user=$(id -u):$(id -g) --volume /etc/passwd:/etc/passwd:ro"
else
  CONTAINER_USER_NAME="builder"
  DOCKER_USER_FLAGS="--user=$CONTAINER_USER_NAME"
fi

# Local paths
LOCAL_WORKDIR="$(pwd)"
LOCAL_CACHE="$HOME/.cache/das/$HOST_ARCH"

LOCAL_BIN_DIR="$LOCAL_WORKDIR/bin/$HOST_ARCH"
LOCAL_LIB_DIR="$LOCAL_WORKDIR/lib/$HOST_ARCH"
LOCAL_PKG_DIR="$LOCAL_WORKDIR/pkg/$HOST_ARCH"

mkdir -p "$LOCAL_CACHE"

# Container paths
IMAGE_NAME="das-builder:${HOST_ARCH:-latest}"
CONTAINER_WORKDIR=/opt/das
CONTAINER_WORKSPACE_DIR=/opt/das/src
CONTAINER_CACHE="/home/${CONTAINER_USER_NAME}/.cache"

# Checks if it's package mode and sets volumes and environment variables
if [[ -z "$PACKAGE_TYPE" ]]; then
  mkdir -p "$LOCAL_BIN_DIR" "$LOCAL_LIB_DIR"

  CONTAINER_BIN_DIR="$CONTAINER_WORKDIR/bin/$HOST_ARCH"
  CONTAINER_LIB_DIR="$CONTAINER_WORKDIR/lib/$HOST_ARCH"

  DOCKER_ENVS="
    -e BIN_DIR=$CONTAINER_BIN_DIR
    -e LIB_DIR=$CONTAINER_LIB_DIR
  "

  BUILD_CMD="./scripts/bazel_build.sh $*"

  DOCKER_VOLUMES="
    --volume $LOCAL_BIN_DIR:$CONTAINER_BIN_DIR
    --volume $LOCAL_LIB_DIR:$CONTAINER_LIB_DIR
  "
else

  if [[ "$PACKAGE_TYPE" != "deb" && "$PACKAGE_TYPE" != "rpm" ]]; then
    echo "ERROR: Invalid package type: use 'deb' ou 'rpm'"
    exit 1
  fi

  mkdir -p "$LOCAL_PKG_DIR"

  CONTAINER_PKG_DIR="$CONTAINER_WORKDIR/pkg/$HOST_ARCH"

  DOCKER_ENVS="
    -e PKG_DIR=$CONTAINER_PKG_DIR
  "

  BUILD_CMD="./scripts/bazel_package.sh $PACKAGE_TYPE"

  DOCKER_VOLUMES="
    --volume $LOCAL_PKG_DIR:$CONTAINER_PKG_DIR
  "
fi

# Docker run
docker run --rm \
  $DOCKER_USER_FLAGS \
  --privileged \
  --platform "$TARGET_PLATFORM" \
  --name="$CONTAINER_NAME" \
  $DOCKER_ENVS \
  $ENV_VARS \
  --network host \
  --volume "$LOCAL_CACHE:$CONTAINER_CACHE" \
  --volume "$LOCAL_WORKDIR:$CONTAINER_WORKDIR" \
  $DOCKER_VOLUMES \
  --workdir "$CONTAINER_WORKSPACE_DIR" \
  "$IMAGE_NAME" \
  $BUILD_CMD

sleep 1

if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
  docker rm -f "${CONTAINER_NAME}" >/dev/null 2>&1 || true
fi
