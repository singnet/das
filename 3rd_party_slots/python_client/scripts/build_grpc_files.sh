#!/bin/bash

mkdir -p $(pwd)/python_bus_client/_grpc

poetry run \
    python -m grpc_tools.protoc \
    --proto_path=$(pwd)/proto \
    --python_out=$(pwd)/python_bus_client/_grpc \
    --pyi_out=$(pwd)/python_bus_client/_grpc \
    --grpc_python_out=$(pwd)/python_bus_client/_grpc \
    $(pwd)/proto/atom_space_node.proto \
    $(pwd)/proto/common.proto

find "$(pwd)/python_bus_client/_grpc" -name '*_pb2*.py' | while read -r file; do
    sed -i \
        -e "s/import atom_space_node_pb2/import python_bus_client._grpc.atom_space_node_pb2/g" \
        -e "s/import common_pb2/import python_bus_client._grpc.common_pb2/g" \
        "$file"
done