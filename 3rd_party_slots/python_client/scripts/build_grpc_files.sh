#!/bin/bash

mkdir -p $(pwd)/hyperon_das/_grpc

poetry run \
    python -m grpc_tools.protoc \
    --proto_path=$(pwd)/proto \
    --python_out=$(pwd)/hyperon_das/_grpc \
    --pyi_out=$(pwd)/hyperon_das/_grpc \
    --grpc_python_out=$(pwd)/hyperon_das/_grpc \
    $(pwd)/proto/atom_space_node.proto \
    $(pwd)/proto/common.proto

find "$(pwd)/hyperon_das/_grpc" -name '*_pb2*.py' | while read -r file; do
    sed -i \
        -e "s/import atom_space_node_pb2/import hyperon_das._grpc.atom_space_node_pb2/g" \
        -e "s/import common_pb2/import hyperon_das._grpc.common_pb2/g" \
        "$file"
done