#!/bin/bash

CONTAINER_NAME="das-builder-bash"

PARAMS="bash"

if [ $# -gt 0 ]; then
    PARAMS=$@
fi

docker run --rm \
    --net="host" \
    --name=$CONTAINER_NAME \
    --volume /tmp:/tmp \
    --volume .:/opt/das \
    -it das-builder \
    $PARAMS

sleep 1
