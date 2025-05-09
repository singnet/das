#!/bin/bash

IMAGE_NAME="das-builder"
CONTAINER_NAME="das-builder-debug"

ENV_VARS=$(test -f .env && echo "--env-file=.env" || echo "")

PARAMS="bash"

if [ $# -gt 0 ]; then
    PARAMS=$@
fi

if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
  echo "Removing existing container: ${CONTAINER_NAME}"
  _=$(docker rm -f "${CONTAINER_NAME}" 2>&1 > /dev/null || true)
fi

docker run --rm -d \
    --net="host" \
    --name=$CONTAINER_NAME \
    $ENV_VARS \
    --volume /tmp:/tmp \
    --volume .:/opt/das \
    --cap-add=SYS_PTRACE \
    --security-opt seccomp=unconfined \
    -it "${IMAGE_NAME}" \
    $PARAMS


sleep 1

echo "Running container: ${CONTAINER_NAME}"
