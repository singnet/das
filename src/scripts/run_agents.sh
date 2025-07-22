#!/bin/bash
# set -e
PWD=$(pwd)


RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[0;33m'
CYAN='\033[0;36m'
ORANGE='\033[38;5;214m'
LIGHT_CYAN='\033[38;5;44m'
PURPLE='\033[38;5;141m'
GRAY='\033[38;5;250m'
PINK='\033[38;5;205m'
NC='\033[0m'
colors=("$GREEN" "$PURPLE" "$YELLOW" "$LIGHT_CYAN" "$PINK" "$GRAY")

# Default values for environment variables
## Agents
ATTENTION_BROKER_PORT=${ATTENTION_BROKER_PORT:-37007}
LINK_CREATION_AGENT_NODE_ID=${LINK_CREATION_AGENT_NODE_ID:-"localhost:9080"}
QUERY_AGENT_PORT=${QUERY_AGENT_PORT:-"31700"}
QUERY_AGENT_NODE_ID=${QUERY_AGENT_SERVER_ID:-"localhost:$QUERY_AGENT_PORT"}
INFERENCE_AGENT_NODE_ID=${INFERENCE_AGENT_NODE_ID:-"localhost:4000"}
EVOLUTION_PORT=${EVOLUTION_PORT:-"7080"}
QUERY_AGENT_START_END_PORT=${QUERY_AGENT_START_END_PORT:-"32700:33700"}
INFERENCE_AGENT_START_END_PORT=${INFERENCE_AGENT_START_END_PORT:-"34700:35700"}
EVOLUTION_START_END_PORT=${EVOLUTION_START_END_PORT:-"36700:37700"}
LINK_CREATION_START_END_PORT=${LINK_CREATION_START_END_PORT:-"37700:38700"}

## Link Creation Agent
LINK_CREATION_QUERY_TIMEOUT_SECONDS=${LINK_CREATION_QUERY_TIMEOUT_SECONDS:-18000}
LINK_CREATION_REQUESTS_INTERVAL_SECONDS=${LINK_CREATION_REQUESTS_INTERVAL_SECONDS:-18000}
LINK_CREATION_REQUESTS_BUFFER_FILE=${LINK_CREATION_REQUESTS_BUFFER_FILE:-"buffer"}
LINK_CREATION_AGENT_THREAD_COUNT=${LINK_CREATION_AGENT_THREAD_COUNT:-1}
LINK_CREATION_METTA_FILE_PATH=${LINK_CREATION_METTA_FILE_PATH:-"/opt/das/bin"}
SAVE_LINKS_TO_METTA=${SAVE_LINKS_TO_METTA:-"true"}

## Other Params
DAS_REDIS_HOSTNAME=${DAS_REDIS_HOSTNAME:-"localhost"}
DAS_REDIS_PORT=${DAS_REDIS_PORT:-6379}
DAS_MONGODB_HOSTNAME=${DAS_MONGODB_HOSTNAME:-"localhost"}
DAS_MONGODB_PORT=${DAS_MONGODB_PORT:-27017}
DAS_MONGODB_USERNAME=${DAS_MONGODB_USERNAME:-"admin"}
DAS_MONGODB_PASSWORD=${DAS_MONGODB_PASSWORD:-"admin"}
DISABLE_ATOMDB_CACHE=${DAS_DISABLE_ATOMDB_CACHE:-"false"}
SAVE_LINKS_TO_DB=${SAVE_LINKS_TO_DB:-"false"}

export DAS_REDIS_HOSTNAME=$DAS_REDIS_HOSTNAME
export DAS_REDIS_PORT=$DAS_REDIS_PORT
export DAS_MONGODB_HOSTNAME=$DAS_MONGODB_HOSTNAME
export DAS_MONGODB_PORT=$DAS_MONGODB_PORT
export DAS_MONGODB_USERNAME=$DAS_MONGODB_USERNAME
export DAS_MONGODB_PASSWORD=$DAS_MONGODB_PASSWORD
export DAS_DISABLE_ATOMDB_CACHE=$DISABLE_ATOMDB_CACHE


# Link Creation Agent params
mkdir -p bin


# DAS Agent params
DAS_AGENT_CONFIG_JSON=$(cat <<EOF
\{\"node_id\":\"$DAS_AGENT_NODE_ID\",\"mongo_hostname\":\"$DAS_MONGODB_HOSTNAME\",\"mongo_port\":$DAS_MONGODB_PORT,\"mongo_username\":\"$DAS_MONGODB_USERNAME\",\"mongo_password\":\"$DAS_MONGODB_PASSWORD\",\"redis_hostname\":\"$DAS_REDIS_HOSTNAME\",\"redis_port\":$DAS_REDIS_PORT\}
EOF
)
# Json string
DAS_AGENT_CONFIG=$(echo $DAS_AGENT_CONFIG_JSON)

# List of agent and paths
AGENTS=(
    "attention_broker_service $ATTENTION_BROKER_PORT;src/scripts/run.sh"
    "query_broker $QUERY_AGENT_PORT $QUERY_AGENT_START_END_PORT;src/scripts/run.sh"
    "link_creation_server $LINK_CREATION_AGENT_NODE_ID $QUERY_AGENT_NODE_ID $LINK_CREATION_START_END_PORT $LINK_CREATION_REQUESTS_INTERVAL_SECONDS $LINK_CREATION_AGENT_THREAD_COUNT $LINK_CREATION_QUERY_TIMEOUT_SECONDS $LINK_CREATION_REQUESTS_BUFFER_FILE $LINK_CREATION_METTA_FILE_PATH $SAVE_LINKS_TO_METTA $SAVE_LINKS_TO_DB;src/scripts/run.sh"
    "inference_agent_server $INFERENCE_AGENT_NODE_ID $QUERY_AGENT_NODE_ID $INFERENCE_AGENT_START_END_PORT;src/scripts/run.sh"
    "evolution_broker $EVOLUTION_PORT $EVOLUTION_START_END_PORT $QUERY_AGENT_NODE_ID;src/scripts/run.sh"
)

WAIT_FOR_AGENTS=true
if [ "$2" == "no-wait" ]; then
    WAIT_FOR_AGENTS=false
fi

SHOW_LOGS=true
if [ "$3" == "no-logs" ]; then
    SHOW_LOGS=false
fi

# stop function
stop() {
   echo "Stopping all agents..."
    for AGENT in "${AGENTS[@]}"; do
        # Split the agent and path
        IFS=';' read -r AGENT_NAME AGENT_PATH <<< "$AGENT"
        echo "Stopping agent: $AGENT_NAME"
        # split AGENT_NAME by space
        if [[ "$AGENT_PATH" == *"src/scripts/run.sh"* ]]; then
            IFS=' ' read -r TEMP_NAME _ _ <<< "$AGENT_NAME"
            echo "$TEMP_NAME"
            CONTAINER_NAME=`docker ps | grep $TEMP_NAME | awk '{print $NF}'`
            # docker rm -f "$CONTAINER_NAME"
        else
            # IFS=' ' read -r _ TEMP_NAME _ <<< "$AGENT_NAME"
            CONTAINER_NAME=`docker ps | grep das-bazel-cmd | awk '{print $NF}'`
            # CONTAINER_NAME=${CONTAINER_NAME//[^[:alnum:]]/}
        fi
        if [ -z "$CONTAINER_NAME" ]; then
            # echo -e "${RED}No container found for agent: $AGENT_NAME${NC}"
            # echo "Container name: $CONTAINER_NAME"
            continue
        fi
        docker rm -f "$CONTAINER_NAME"
    done
}

trap stop SIGINT
trap stop SIGTERM

PARAM=$1
set +x
if [ "$PARAM" == "stop" ]; then
    stop
    exit 0
fi
if [ "$PARAM" == "start" ]; then
    echo "Starting all agents..."
    i=0
    for AGENT in "${AGENTS[@]}"; do
        # Split the agent and path
        IFS=';' read -r AGENT_NAME AGENT_PATH <<< "$AGENT"
        IFS=' ' read -r TEMP_NAME _ _ <<< "$AGENT_NAME"
        # create a log file
        LOG_FILE="$PWD/logs/$TEMP_NAME.log"
        mkdir -p logs
        touch $LOG_FILE
        # echo "Starting agent: $AGENT_NAME"
        {
            echo -e "${colors[i % ${#colors[@]}]}Starting: $AGENT_NAME ${commands[i]}${NC}"
            bash -c "$PWD/$AGENT_PATH $AGENT_NAME" 2>&1 | while IFS= read -r line; do
                # show log
                if [ "$SHOW_LOGS" = true ]; then
                    echo -e "${colors[i % ${#colors[@]}]}[$TEMP_NAME] $line${NC}"
                fi
                echo "$line" >> $LOG_FILE
            done
        } &
        sleep 5
        i=$((i + 1))
    done
    if [ "$WAIT_FOR_AGENTS" = true ]; then
        echo -e "${GREEN}All agents started successfully!${NC}"
        echo "Waiting for agents to finish..."
        wait
    else
        echo -e "${GREEN}All agents started in the background!${NC}"
    fi
fi
