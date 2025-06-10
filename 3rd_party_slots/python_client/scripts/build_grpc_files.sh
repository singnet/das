#!/bin/bash

mkdir -p $(pwd)/hyperon_das/_grpc

if [ -z "$1" ]
then
    echo "Usage: build_grpc_files.sh PROTO_DIR"
    exit 1
else
    PROTO_DIR=$1
fi

# git submodule update --init --recursive

poetry run \
    python -m grpc_tools.protoc \
    --proto_path="$PROTO_DIR" \
    --python_out=$(pwd)/hyperon_das/_grpc \
    --pyi_out=$(pwd)/hyperon_das/_grpc \
    --grpc_python_out=$(pwd)/hyperon_das/_grpc \
    "$PROTO_DIR"/atom_space_node.proto \
    "$PROTO_DIR"/common.proto

find "$(pwd)/hyperon_das/_grpc" -name '*_pb2*.py' | while read -r file; do
    sed -i \
        -e "s/import atom_space_node_pb2/import hyperon_das._grpc.atom_space_node_pb2/g" \
        -e "s/import common_pb2/import hyperon_das._grpc.common_pb2/g" \
        "$file"
done