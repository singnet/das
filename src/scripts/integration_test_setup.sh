#!/bin/bash

WATCH_DIR="./bin"
START_FILE="$WATCH_DIR/start"
END_FILE="$WATCH_DIR/end"
KILL_FILE="$WATCH_DIR/kill"
RUNNING_FILE="$WATCH_DIR/running"

echo "Waiting commands in $WATCH_DIR..."

ENV_VALUES="DAS_REDIS_HOSTNAME=localhost
DAS_REDIS_PORT=6379
DAS_USE_REDIS_CLUSTER=false,
DAS_MONGODB_HOSTNAME=localhost
DAS_MONGODB_PORT=27017
DAS_MONGODB_USERNAME=root
DAS_MONGODB_PASSWORD=root
ATTENTION_BROKER_PORT=37007
QUERY_AGENT_PORT=1200
QUERY_AGENT_START_END_PORT=\"1300:1400\"
INFERENCE_AGENT_NODE_ID=\"localhost:1500\"
INFERENCE_AGENT_START_END_PORT=\"1600:1700\"
LINK_CREATION_AGENT_NODE_ID=\"localhost:1800\"
LINK_CREATION_START_END_PORT=\"1900:2000\"
EVOLUTION_PORT=2100
EVOLUTION_START_END_PORT=\"2200:2300\""

while true; do


    if [ -f "$END_FILE" -o -f "$KILL_FILE" ]; then
        echo "Running end at $(date)"
        if [ -f "$END_FILE" ]; then
            rm -f "$END_FILE"
        fi
        if [ -f "$RUNNING_FILE" ]; then
            rm -f "$RUNNING_FILE"
        fi
        if [ -f ".env.bak" ]; then
             mv .env.bak .env
        fi
        ./src/scripts/run_agents.sh stop >> /dev/null
        docker compose -f src/tests/integration/cpp/data/compose_db.yaml down > /dev/null 2>&1
        rm -f ./logs/*.log
        if [ -f "$KILL_FILE" ]; then
            rm -f "$KILL_FILE"
            echo "Exiting integration test setup."
            exit 0
        fi
    fi

    if [ -f "$START_FILE" ]; then
        rm -f "$START_FILE"
        echo "Running start at $(date)"
        mv .env .env.bak
        echo "$ENV_VALUES" > .env
        docker compose -f src/tests/integration/cpp/data/compose_db.yaml up -d --build > /dev/null 2>&1
        set -a 
        chmod +x ./src/scripts/run_agents.sh 
        source .env
        set +a 
        ./src/scripts/run_agents.sh start no-wait no-logs >> /dev/null
        echo "Agents started."
        touch "$RUNNING_FILE"
    fi

    sleep 1
done