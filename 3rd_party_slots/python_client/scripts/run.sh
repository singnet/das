#!/bin/bash

mkdir -p $(pwd)/dist
mkdir -p $(pwd)/python_bus_client/_grpc
docker run --rm -v $(pwd):/app python-client bash -c "poetry install && /app/scripts/build.sh"