#!/bin/bash

if [ -z "$1" ]
then
    echo "Usage: redis-down.sh PORT"
    exit 1
else
    PORT=$1
fi

echo "Destroying Redis container on port $PORT"

docker stop redis_$PORT && docker rm redis_$PORT
