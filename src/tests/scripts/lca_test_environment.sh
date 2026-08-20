#!/bin/bash

# Usage: NO COMMAND LINE PARAMETERS

KBGZ="inference_toy_1000_5_3_abcde_100_90.metta.gz"
KB="${KBGZ%.*}"

cp "$(dirname "$0")"/../assets/$KBGZ /tmp
gunzip --quiet --force /tmp/$KBGZ
das-cli db stop --prune >> /tmp/atomdb.log 2>&1
(docker stop `docker ps | tail -n +2 | cut -d" " -f1` ; docker container prune -f ; docker ps -a) > /dev/null 2>&1
das-cli db start >> /tmp/atomdb.log 2>&1
make run-db-loader OPTIONS="--config=/opt/das/config/das.json --file=/tmp/$KB --threads=4 --chunk=5000" >> /tmp/atomdb.log 2>&1
make run-attention-broker >> /tmp/attention_broker.log 2>&1 &
sleep 5
make run-busnode OPTIONS="--service=query-engine --config=/opt/das/config/das.json" >> /tmp/query_engine.log 2>&1 &
sleep 5
make run-busnode OPTIONS="--service=link-creation-agent --bus-endpoint=localhost:40002 --config=/opt/das/config/das.json" >> /tmp/link_creation_agent.log 2>&1 &
sleep 5
