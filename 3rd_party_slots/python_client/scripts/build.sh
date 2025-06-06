#!/bin/bash

DIR="/app"

mkdir -p "$DIR"/python_bus_client/_grpc

poetry run \
    python -m grpc_tools.protoc \
    --proto_path=/proto_files \
    --python_out="$DIR"/python_bus_client/_grpc \
    --pyi_out="$DIR"/python_bus_client/_grpc \
    --grpc_python_out="$DIR"/python_bus_client/_grpc \
    /proto_files/atom_space_node.proto \
    /proto_files/common.proto

# Change imports to grpc files (python_bus_client._grpc)
find "$DIR/python_bus_client/_grpc" -name '*_pb2*.py' | while read -r file; do
    sed -i \
        -e "s/import atom_space_node_pb2/import python_bus_client._grpc.atom_space_node_pb2/g" \
        -e "s/import common_pb2/import python_bus_client._grpc.common_pb2/g" \
        "$file"
done

poetry build --format wheel

rm -Rf "$DIR"/python_bus_client/_grpc