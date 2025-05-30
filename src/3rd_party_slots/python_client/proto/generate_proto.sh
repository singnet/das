#!/bin/bash

python3 -m grpc_tools.protoc \
    -Iproto \
    --python_out=. \
    --grpc_python_out=. \
    proto/atom_space_node.proto

