#!/bin/bash -x

docker buildx build -t das-builder --load -f src/docker/Dockerfile .
