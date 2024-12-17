#!/bin/bash

CONTAINER_NAME="das-attention-broker-build"

mkdir -p bin
docker run \
    --name=$CONTAINER_NAME \
    --volume .:/opt/das-attention-broker \
    --workdir /opt/das-attention-broker \
    das-attention-broker-builder \
    ./bin/attention_broker $1

sleep 1
docker rm $CONTAINER_NAME
