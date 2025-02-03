#!/bin/bash -x

docker buildx build -t das-spawner-client-builder --load -f docker/Dockerfile.spawner .