#!/bin/bash

PROTOS=("echo" "common" "attention_broker")

for proto in ${PROTOS[@]}; do
    protoc -I/opt/proto --cpp_out=/opt/grpc /opt/proto/${proto}.proto
    cd /opt/grpc
    gcc -c ${proto}.pb.cc
done
