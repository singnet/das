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

ARCH=$(detect_host_arch)

BINARY_NAME="${1}"
shift

IMAGE_NAME="das-builder:${ARCH}"
CONTAINER_NAME="das-${BINARY_NAME}-$(uuidgen | cut -d '-' -f 1)-$(date +%Y%m%d%H%M%S)"

ENV_VARS=$(test -f .env && echo "--env-file=.env" || echo "")

mkdir -p bin
docker run --rm \
    --name="${CONTAINER_NAME}" \
    --network host \
    --volume .:/opt/das \
    --workdir /opt/das \
    $ENV_VARS \
    "${IMAGE_NAME}" \
    "bin/$ARCH/${BINARY_NAME}" "$@"

sleep 1

if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
  echo "Removing existing container: ${CONTAINER_NAME}"
  _=$(docker rm -f "${CONTAINER_NAME}" 2>&1 > /dev/null || true)
fi
