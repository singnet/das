#!/usr/bin/env bash

set -euo pipefail

export LC_NUMERIC=C

TESTS_ROUNDS=10
FAILED_TIME=-1.0
ANY_ROUND_FAILED=false

set_dot_env_file() {
  cat > .env <<EOF
DAS_MONGODB_HOSTNAME=localhost
DAS_MONGODB_PORT=38000
DAS_MONGODB_USERNAME=dbadmin
DAS_MONGODB_PASSWORD=dassecret
DAS_REDIS_HOSTNAME=localhost
DAS_REDIS_PORT=39000
EOF
}

force_stop() {
  local pattern="$1"
  case "$pattern" in
    *run-attention-broker*) pattern="attention_broker" ;;
    *run-query-agent*) pattern="query_broker" ;;
    *) return ;;
  esac
  docker rm -f $(docker ps -a | awk "/$pattern/ {print \$1}") 2>/dev/null || true
}

start_process() {
  local command="$1"
  force_stop "$command"
  nohup bash -c "$command" >/dev/null 2>&1 &
  echo $!
}

stop_process() {
  local pid="$1"
  kill -TERM "-$pid" 2>/dev/null || true
  wait "$pid" 2>/dev/null || true
}

run_command() {
  local command="$1"
  local start_time
  local end_time

  start_time=$(date +%s.%N)
  if eval "$command" >/dev/null 2>&1; then
    end_time=$(date +%s.%N)
    echo "$(echo "$end_time - $start_time" | bc)"
  else
    echo "$FAILED_TIME"
  fi
}

main() {
  set_dot_env_file

  declare -A queries=(
    ["linktemplate_3_node_var_link"]="LINK_TEMPLATE Expression 3 NODE Symbol Contains VARIABLE sentence1 LINK Expression 2 NODE Symbol Word NODE Symbol '\"aaa\"'"
    ["and_2_linktemplate_linktemplate"]="AND 2 LINK_TEMPLATE Expression 3 NODE Symbol Contains VARIABLE sentence1 LINK Expression 2 NODE Symbol Word NODE Symbol '\"bbb\"' LINK_TEMPLATE Expression 3 NODE Symbol Contains VARIABLE sentence2 LINK Expression 2 NODE Symbol Word NODE Symbol '\"aaa\"'"
    ["and_2_linktemplate_or_2_linktemplate_linktemplate"]="AND 2 LINK_TEMPLATE Expression 3 NODE Symbol Contains VARIABLE sentence1 LINK Expression 2 NODE Symbol Word NODE Symbol '\"bbb\"' OR 2 LINK_TEMPLATE Expression 3 NODE Symbol Contains VARIABLE sentence2 LINK Expression 2 NODE Symbol Word NODE Symbol '\"aaa\"' LINK_TEMPLATE Expression 3 NODE Symbol Contains VARIABLE sentence3 LINK Expression 2 NODE Symbol Word NODE Symbol '\"ccc\"'"
  )

  cmd_prefix="bash src/scripts/run.sh query 'localhost:31701' 'localhost:31700' false 1"
  cmd_suffix=""

  echo "Starting Attention Broker..."
  attention_broker_pid=$(start_process "make run-attention-broker")
  sleep 3

  query_agent_pid=$(start_process "make run-query-agent")
  sleep 3
  stop_process "$query_agent_pid"
  sleep 3

  for name in "${!queries[@]}"; do
    query="${queries[$name]}"
    echo ""
    echo "Running query '$name'..."

    total_time=0.0
    valid_rounds=$TESTS_ROUNDS

    for (( round=0; round<TESTS_ROUNDS; round++ )); do
      query_agent_pid=$(start_process "make run-query-agent")
      sleep 3

      echo -n "  $round: "

      round_time=$(run_command "$cmd_prefix $query $cmd_suffix")

      stop_process "$query_agent_pid"

      if [[ "$round_time" != "$FAILED_TIME" ]]; then
        total_time=$(echo "$total_time + $round_time" | bc)
        printf "%.2f seconds\n" "$round_time"
      else
        echo "Failed"
        valid_rounds=$((valid_rounds - 1))
        ANY_ROUND_FAILED=true
      fi
    done

    if (( valid_rounds > 0 )); then
      avg_time=$(echo "$total_time / $valid_rounds" | bc -l)
      printf "Average time for '%s': %.2f seconds (over %d rounds)\n" "$name" "$avg_time" "$valid_rounds"
    else
      echo "All rounds failed for '$name'."
    fi
  done

  echo "Stopping Attention Broker..."
  stop_process "$attention_broker_pid"

  if [ "$ANY_ROUND_FAILED" = true ]; then
    exit 1
  else
    exit 0
  fi
}

main
