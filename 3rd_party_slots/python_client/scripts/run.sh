#!/bin/bash

mkdir -p $(pwd)/dist

mkdir -p $(pwd)/hyperon_das/_grpc

docker run --rm -v $(pwd):/app python-client bash -c "poetry lock --no-update && poetry install && /app/scripts/build.sh"