#!/bin/bash

if [ -z "$1" ]
then
    echo "Usage: redis-up.sh PORT"
    exit 1
else
    PORT=$1
    CLUSTER_ENABLED="no"
fi

CONFIG_FILE="/tmp/redis_$PORT.conf"
cp ./redis.conf $CONFIG_FILE

cat <<EOM >$CONFIG_FILE
cluster-config-file redis-cluster-state.conf
cluster-node-timeout 30000
appendonly yes
protected-mode no
EOM

echo "cluster-enabled $CLUSTER_ENABLED" >> $CONFIG_FILE
echo "port $PORT" >> $CONFIG_FILE

echo "Starting Redis on port $PORT"

docker stop redis_$PORT >& /dev/null
docker rm redis_$PORT >& /dev/null

docker run \
    --detach \
    --name redis_$PORT \
    --env TZ=${TZ} \
    --network="host" \
    --volume /tmp:/tmp \
    --volume /mnt:/mnt \
    redis/redis-stack-server:latest \
    redis-server $CONFIG_FILE
