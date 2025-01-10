#!/bin/bash

if [ -z "$1" ]
then
    echo "Usage: mongo-down.sh PORT"
    exit 1
else
    PORT=$1
fi

echo "Destroying MongoDB container on port $PORT"

docker stop mongo_$PORT && \
  docker kill mongo_$PORT &&
  docker rm mongo_$PORT && \
  docker volume rm mongodbdata_$PORT >& /dev/null
