#!/usr/bin/env bash

set -euo pipefail

# Number of times to run each query
TESTS_ROUNDS=10
ENV_FILE=".env"

# Environment Variables
export DAS_MONGODB_HOSTNAME="localhost"
export DAS_MONGODB_PORT="38000"
export DAS_MONGODB_USERNAME="dbadmin"
export DAS_MONGODB_PASSWORD="dassecret"
export DAS_REDIS_HOSTNAME="localhost"
export DAS_REDIS_PORT="39000"

create_or_update_env_file() {
  {
    echo "DAS_MONGODB_HOSTNAME=$DAS_MONGODB_HOSTNAME"
    echo "DAS_MONGODB_PORT=$DAS_MONGODB_PORT"
    echo "DAS_MONGODB_USERNAME=$DAS_MONGODB_USERNAME"
    echo "DAS_MONGODB_PASSWORD=$DAS_MONGODB_PASSWORD"
    echo "DAS_REDIS_HOSTNAME=$DAS_REDIS_HOSTNAME"
    echo "DAS_REDIS_PORT=$DAS_REDIS_PORT"
  } > "$ENV_FILE"
}

start_process() {
  local cmd="$1"

  (
    $cmd
  ) > /tmp/${cmd// /_}.log 2>&1 &

  echo $!
}

stop_process() {
  local pid=$1

  if kill -0 "$pid" 2>/dev/null; then
    kill "$pid"
    wait "$pid" 2>/dev/null || true
    echo "Process $pid stopped."
  else
    echo "Process $pid does not exist."
  fi
}

run_command() {
  local command=$1
  local start end

  start=$(date +%s.%N)
  if ! eval "$command" > /dev/null 2>&1; then
    echo "Command failed: $command"
    exit 1
  fi
  end=$(date +%s.%N)
  awk -v start="$start" -v end="$end" 'BEGIN { print end - start }'
}

main() {
  create_or_update_env_file

  declare -A queries=(
    ["linktemplate_3_node_var_link"]="LINK_TEMPLATE Expression 3 NODE Symbol Contains VARIABLE sentence1 LINK Expression 2 NODE Symbol Word NODE Symbol '\"aaa\"'"
    ["and_2_linktemplate_linktemplate"]="AND 2 LINK_TEMPLATE Expression 3 NODE Symbol Contains VARIABLE sentence1 LINK Expression 2 NODE Symbol Word NODE Symbol '\"bbb\"' LINK_TEMPLATE Expression 3 NODE Symbol Contains VARIABLE sentence2 LINK Expression 2 NODE Symbol Word NODE Symbol '\"aaa\"'"
    ["and_2_linktemplate_or_2_linktemplate_linktemplate"]="AND 2 LINK_TEMPLATE Expression 3 NODE Symbol Contains VARIABLE sentence1 LINK Expression 2 NODE Symbol Word NODE Symbol '\"bbb\"' OR 2 LINK_TEMPLATE Expression 3 NODE Symbol Contains VARIABLE sentence2 LINK Expression 2 NODE Symbol Word NODE Symbol '\"aaa\"' LINK_TEMPLATE Expression 3 NODE Symbol Contains VARIABLE sentence3 LINK Expression 2 NODE Symbol Word NODE Symbol '\"ccc\"'"
  )

  local cmd_prefix="bash src/scripts/run.sh query 'localhost:31701' 'localhost:31700' false "
  local cmd_suffix=""

  echo "Starting Attention Broker..."
  broker_pid=$(start_process "make run-attention-broker")
  sleep 3

  echo "Warm-up Query Agent..."
  qa_pid=$(start_process "make run-query-agent")
  sleep 3
  stop_process "$qa_pid"
  sleep 3

  for name in "${!queries[@]}"; do
    query="${queries[$name]}"
    echo ""
    echo "Running query '$name'..."
    total_time=0

    echo "Rounds [for round in range($TESTS_ROUNDS)]:"
    for ((round = 0; round < TESTS_ROUNDS; round++)); do
      qa_pid=$(start_process "make run-query-agent")
      sleep 3

      echo -n "  $round: "
      round_time=$(run_command "$cmd_prefix \"$query\" $cmd_suffix")
      stop_process "$qa_pid"

      printf "%.2f seconds\n" "$round_time"
      total_time=$(awk -v a="$total_time" -v b="$round_time" 'BEGIN { print a + b }')
    done

    avg=$(awk -v total="$total_time" -v rounds="$TESTS_ROUNDS" 'BEGIN { print total / rounds }')
    echo "Average time for '$name': ${avg} seconds"
  done

  echo ""
  echo "Stopping Attention Broker..."
  stop_process "$broker_pid"
}

main "$@"
