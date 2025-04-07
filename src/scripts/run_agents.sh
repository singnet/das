#!/bin/bash
set -e
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
QUERY_AGENT_PORT=${QUERY_AGENT_PORT:-31700}
LINK_CREATION_AGENT_NODE_ID=${LINK_CREATION_AGENT_NODE_ID:-"localhost:9080"}
QUERY_AGENT_NODE_ID=${QUERY_AGENT_SERVER_ID:-"localhost:$QUERY_AGENT_PORT"}
INFERENCE_AGENT_NODE_ID=${INFERENCE_AGENT_NODE_ID:-"localhost:4000"}
DAS_AGENT_NODE_ID=${DAS_AGENT_SERVER_ID:-"localhost:7000"}
EVOLUTION_AGENT_NODE_ID=${EVOLUTION_AGENT_NODE_ID:-"localhost:9080"}

## Clients
LINK_CREATION_QUERY_AGENT_ID=${LINK_CREATION_QUERY_AGENT_ID:-"localhost:9001"}
LINK_CREATION_DAS_AGENT_ID=${LINK_CREATION_DAS_AGENT_ID:-"localhost:7010"}
INFERENCE_AGENT_DAS_AGENT_ID=${INFERENCE_AGENT_DAS_AGENT_ID:-"localhost:9090"}
INFERENCE_AGENT_EVOLUTION_AGENT_ID=${INFERENCE_AGENT_EVOLUTION_AGENT_ID:-"localhost:9081"}
INFERENCE_AGENT_LINK_CREATION_AGENT_ID=${INFERENCE_AGENT_LINK_CREATION_AGENT_ID:-"localhost:9081"}

## Link Creation Agent
LINK_CREATION_QUERY_AGENT_START_PORT=${LINK_CREATION_QUERY_AGENT_START_PORT:-16000}
LINK_CREATION_QUERY_AGENT_END_PORT=${LINK_CREATION_QUERY_AGENT_END_PORT:-16200}
LINK_CREATION_QUERY_TIMEOUT_SECONDS=${LINK_CREATION_QUERY_TIMEOUT_SECONDS:-20}
LINK_CREATION_REQUESTS_INTERVAL_SECONDS=${LINK_CREATION_REQUESTS_INTERVAL_SECONDS:-5}
LINK_CREATION_REQUESTS_BUFFER_FILE=${LINK_CREATION_REQUESTS_BUFFER_FILE:-"buffer"}
LINK_CREATION_AGENT_THREAD_COUNT=${LINK_CREATION_AGENT_THREAD_COUNT:-1}

## Other Params
DAS_REDIS_HOSTNAME=${DAS_REDIS_HOSTNAME:-"localhost"}
DAS_REDIS_PORT=${DAS_REDIS_PORT:-6379}
DAS_MONGODB_HOSTNAME=${DAS_MONGODB_HOSTNAME:-"localhost"}
DAS_MONGODB_PORT=${DAS_MONGODB_PORT:-27017}
DAS_MONGODB_USERNAME=${DAS_MONGODB_USERNAME:-"admin"}
DAS_MONGODB_PASSWORD=${DAS_MONGODB_PASSWORD:-"admin"}

export DAS_REDIS_HOSTNAME=$DAS_REDIS_HOSTNAME
export DAS_REDIS_PORT=$DAS_REDIS_PORT
export DAS_MONGODB_HOSTNAME=$DAS_MONGODB_HOSTNAME
export DAS_MONGODB_PORT=$DAS_MONGODB_PORT
export DAS_MONGODB_USERNAME=$DAS_MONGODB_USERNAME
export DAS_MONGODB_PASSWORD=$DAS_MONGODB_PASSWORD


# Link Creation Agent params
mkdir -p src/bin
LINK_CREATION_FILE_PATH=$PWD/src/bin/link_creation_server.cfg
if [ -f "$LINK_CREATION_FILE_PATH" ]; then
    echo "File $LINK_CREATION_FILE_PATH already exists. Skipping creation."
else
    echo "Creating file $LINK_CREATION_FILE_PATH."
    touch $PWD/src/bin/link_creation_server.cfg
    echo "link_creation_agent_thread_count = $LINK_CREATION_AGENT_THREAD_COUNT" >> $PWD/src/bin/link_creation_server.cfg
    echo "link_creation_agent_server_id = $LINK_CREATION_AGENT_NODE_ID" >> $PWD/src/bin/link_creation_server.cfg
    echo "query_agent_server_id = $QUERY_AGENT_NODE_ID" >> $PWD/src/bin/link_creation_server.cfg
    echo "query_agent_client_id = $LINK_CREATION_QUERY_AGENT_ID" >> $PWD/src/bin/link_creation_server.cfg
    echo "query_agent_client_start_port = $LINK_CREATION_QUERY_AGENT_START_PORT" >> $PWD/src/bin/link_creation_server.cfg
    echo "query_agent_client_end_port = $LINK_CREATION_QUERY_AGENT_END_PORT" >> $PWD/src/bin/link_creation_server.cfg
    echo "das_agent_client_id = $LINK_CREATION_DAS_AGENT_ID" >> $PWD/src/bin/link_creation_server.cfg
    echo "das_agent_server_id = $DAS_AGENT_NODE_ID" >> $PWD/src/bin/link_creation_server.cfg
    echo "query_timeout_seconds = $LINK_CREATION_QUERY_TIMEOUT_SECONDS" >> $PWD/src/bin/link_creation_server.cfg
    echo "requests_interval_seconds = $LINK_CREATION_REQUESTS_INTERVAL_SECONDS" >> $PWD/src/bin/link_creation_server.cfg
    echo "requests_buffer_file = $LINK_CREATION_REQUESTS_BUFFER_FILE" >> $PWD/src/bin/link_creation_server.cfg
    echo "metta_file_path = /opt/das/src/bin" >> $PWD/src/bin/link_creation_server.cfg
fi

# Inference Agent params
INFERENCE_AGENT_FILE_PATH=$PWD/src/bin/inference_agent_server.cfg
if [ -f "$INFERENCE_AGENT_FILE_PATH" ]; then
    echo "File $INFERENCE_AGENT_FILE_PATH already exists. Skipping creation."
else
    echo "Creating file $INFERENCE_AGENT_FILE_PATH."
    touch $PWD/src/bin/inference_agent_server.cfg
    echo "inference_node_id=$INFERENCE_AGENT_NODE_ID" >> $PWD/src/bin/inference_agent_server.cfg
    echo "das_client_id=$INFERENCE_AGENT_DAS_AGENT_ID" >> $PWD/src/bin/inference_agent_server.cfg
    echo "das_server_id=$DAS_AGENT_NODE_ID" >> $PWD/src/bin/inference_agent_server.cfg
    echo "distributed_inference_control_node_id=$INFERENCE_AGENT_EVOLUTION_AGENT_ID" >> $PWD/src/bin/inference_agent_server.cfg
    echo "distributed_inference_control_node_server_id=$EVOLUTION_AGENT_NODE_ID" >> $PWD/src/bin/inference_agent_server.cfg
    echo "link_creation_agent_server_id=$LINK_CREATION_AGENT_NODE_ID" >> $PWD/src/bin/inference_agent_server.cfg
    echo "link_creation_agent_client_id=$INFERENCE_AGENT_LINK_CREATION_AGENT_ID" >> $PWD/src/bin/inference_agent_server.cfg
fi

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
    # "query_broker $QUERY_AGENT_PORT;src/scripts/run.sh"
    "link_creation_server --config_file ./src/bin/link_creation_server.cfg;src/scripts/run.sh"
    "inference_agent_server --config_file ./src/bin/inference_agent_server.cfg;src/scripts/run.sh"
    "run //das_agent:main -- --config $DAS_AGENT_CONFIG;src/scripts/bazel.sh"
)

PARAM=$1
set +x
if [ "$PARAM" == "stop" ]; then
    echo "Stopping all agents..."
    for AGENT in "${AGENTS[@]}"; do
        # Split the agent and path
        IFS=';' read -r AGENT_NAME AGENT_PATH <<< "$AGENT"
        echo "Stopping agent: $AGENT_NAME"
        # split AGENT_NAME by space
        if [[ "$AGENT_PATH" == *"src/scripts/run.sh"* ]]; then
            IFS=' ' read -r CONTAINER_NAME _ <<< "$AGENT_NAME"
            docker rm -f "$CONTAINER_NAME"
        else
            IFS=' ' read -r _ CONTAINER_NAME _ <<< "$AGENT_NAME"
            CONTAINER_NAME=${CONTAINER_NAME//[^[:alnum:]]/}
        fi
        docker rm -f "$CONTAINER_NAME"
        rm -f $LINK_CREATION_FILE_PATH
        rm -f $INFERENCE_AGENT_FILE_PATH
    done
    exit 0
fi
if [ "$PARAM" == "start" ]; then
    echo "Starting all agents..."
    i=0
    for AGENT in "${AGENTS[@]}"; do
        # Split the agent and path
        IFS=';' read -r AGENT_NAME AGENT_PATH <<< "$AGENT"
        # echo "Starting agent: $AGENT_NAME"
        {
            echo -e "${colors[i % ${#colors[@]}]}Starting: $AGENT_NAME ${commands[i]}${NC}"
            bash -c "$PWD/$AGENT_PATH $AGENT_NAME" 2>&1 | while IFS= read -r line; do
                echo -e "${colors[i % ${#colors[@]}]}$line${NC}"
            done
        } &
        sleep 5
        i=$((i + 1))
    done
    exit 0
fi