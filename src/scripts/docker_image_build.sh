#!/bin/bash

docker buildx build -t das-attention-broker-builder --load -f docker/Dockerfile .
