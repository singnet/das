#!/bin/bash -x

docker buildx build -t das-builder --load -f src/docker/Dockerfile .
docker buildx build -t evolution-builder --load -f src/evolution/Dockerfile ./src/evolution
