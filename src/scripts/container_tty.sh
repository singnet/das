#!/bin/bash

CONTAINER_NAME="das-attention-broker-bash"

PARAMS="bash"

if [ $# -gt 0 ]; then
    PARAMS=$@
fi

docker run --rm \
    --net="host" \
    --name=$CONTAINER_NAME \
    --volume /tmp:/tmp \
    --volume .:/opt/das-attention-broker \
    -it das-attention-broker-builder \
    $PARAMS

sleep 1
