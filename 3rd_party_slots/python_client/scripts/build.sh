#!/bin/bash

DIR="/app"

mkdir -p "$DIR"/hyperon_das/_grpc

poetry run \
    python -m grpc_tools.protoc \
    --proto_path=/proto_files \
    --python_out="$DIR"/hyperon_das/_grpc \
    --pyi_out="$DIR"/hyperon_das/_grpc \
    --grpc_python_out="$DIR"/hyperon_das/_grpc \
    /proto_files/atom_space_node.proto \
    /proto_files/common.proto

# Change imports to grpc files (hyperon_das._grpc)
find "$DIR/hyperon_das/_grpc" -name '*_pb2*.py' | while read -r file; do
    sed -i \
        -e "s/import atom_space_node_pb2/import hyperon_das._grpc.atom_space_node_pb2/g" \
        -e "s/import common_pb2/import hyperon_das._grpc.common_pb2/g" \
        "$file"
done

poetry build --format wheel

rm -Rf "$DIR"/hyperon_das/_grpc