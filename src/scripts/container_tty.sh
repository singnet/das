#!/bin/bash

CONTAINER_NAME="das-attention-broker-bash"

docker run \
    --net="host" \
    --name=$CONTAINER_NAME \
    --volume /tmp:/tmp \
    --volume .:/opt/das-attention-broker \
    -it das-attention-broker-builder \
    bash

sleep 1
