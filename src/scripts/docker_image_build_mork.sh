#!/bin/bash -x

docker buildx build -t das-mork-server --load -f src/docker/mork/Dockerfile.server .
docker buildx build -t das-mork-loader --load -f src/docker/mork/Dockerfile.loader .
