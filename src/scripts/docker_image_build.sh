#!/bin/bash -x

docker buildx build -t das-builder --load -f docker/Dockerfile .
