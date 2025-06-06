#!/bin/bash -x

docker buildx build -t python-client --load -f Dockerfile .