#!/bin/bash

AVAIABLE_RAM=110G

KB=$1
CONTEXT_TAG=$2
TARGET_PREDICATE=$3
TARGET_CONCEPT=$4
RENT=$5
SPREAD_LOWER=$6
SPREAD_UPPER=$7
ELITISM=$8
SELECTION=$9

POPULATION_SIZE=100
NUM_GENERATIONS=5
NUM_ITERATIONS=5

echo "--------------------------------------------------"
echo "Cleaning docker containers"
echo
das-cli db stop --prune
docker stop `docker ps | tail -n +2 | cut -d" " -f1` ; docker container prune -f ; docker ps -a

echo
echo "--------------------------------------------------"
echo "Loading knowledge base"
echo
das-cli db start
make run-db-loader OPTIONS="--config=/opt/das/config/das.json --file=$KB --threads=4 --chunk=5000"

echo
echo "--------------------------------------------------"
echo "Stating agents"
echo
make run-attention-broker &>> /tmp/ab.log &
sleep 2
make run-busnode OPTIONS="--service=query-engine --config=/opt/das/config/das.json" &>> /tmp/qa.log &
sleep 2
make run-busnode OPTIONS="--service=evolution-agent --bus-endpoint=localhost:40002 --config=/opt/das/config/das.json" &>> /tmp/ev.log &
sleep 2
docker update -m $AVAIABLE_RAM --memory-swap $AVAIABLE_RAM $(docker ps -q)
echo "Done. Logs are in /tmp/ab.log /tmp/qa.log /tmp/ev.log"

command_line=(src/scripts/run.sh evaluation_evolution localhost:35000 localhost:40002 35001:35999 /opt/das/config/das.json $CONTEXT_TAG "$TARGET_PREDICATE" "$TARGET_CONCEPT" $RENT $SPREAD_LOWER $SPREAD_UPPER $ELITISM $SELECTION $POPULATION_SIZE $NUM_GENERATIONS $NUM_ITERATIONS)
echo
echo "--------------------------------------------------"
echo "Running <${command_line[@]}>"
echo
echo "${#command_line[@]}"
echo
"${command_line[@]}"
