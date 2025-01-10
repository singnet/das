#!/bin/bash

if [ -z "$1" ]
then
    echo "Usage: mongo-up.sh PORT"
    exit 1
else
    PORT=$1
fi

echo "Starting MongoDB on port $PORT"

docker stop mongo_$PORT >& /dev/null
docker rm mongo_$PORT >& /dev/null
docker volume rm mongodbdata_$PORT >& /dev/null

sleep 1
docker run \
    --detach \
    --name mongo_$PORT \
    --env MONGO_INITDB_ROOT_USERNAME="dbadmin" \
    --env MONGO_INITDB_ROOT_PASSWORD="dassecret" \
    --env TZ=${TZ} \
    --network="host" \
    --volume /tmp:/tmp \
    --volume /mnt:/mnt \
    --volume mongodbdata_$PORT:/data/db \
    mongo:latest \
    mongod --port $PORT
