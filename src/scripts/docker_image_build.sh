#!/bin/bash -x

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

TARGET_ARCHS="${1:-$HOST_ARCH}"

IFS=',' read -r -a TARGET_ARCHS_ARRAY <<< "$TARGET_ARCHS"

for TARGET_ARCH in "${TARGET_ARCHS_ARRAY[@]}"; do
  IMAGE_TAG="das-builder:$TARGET_ARCH"
  PLATFORM="linux/$TARGET_ARCH"
  
  docker buildx build \
    -t "${IMAGE_TAG}" \
    --load \
    --platform "${PLATFORM}" \
    -f src/docker/Dockerfile .

  if [ "$TARGET_ARCH" == "$HOST_ARCH" ]; then
    docker tag "${IMAGE_TAG}" das-builder:latest
  fi
done