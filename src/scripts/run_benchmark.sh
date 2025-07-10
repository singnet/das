#!/usr/bin/env bash

set -eou pipefail

DB=""
REL=""
CONCURRENCY="1"
CACHE="disabled"
ITERATIONS=10
SENTENCES=""
WORD_COUNT=""
WORD_LENGTH=""
ALPHABET_RANGE=""
METTA_PATH="/tmp/${RANDOM}_$(date +%Y%m%d%H%M%S).metta"
RESET='\033[0m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
GREEN='\033[0;32m'


usage() {
    echo "Usage: $0 [OPTIONS]"
    echo
    echo "  --db           Database size: empty | small | medium | large | xlarge"
    echo "  --rel          Atoms relationships: loosely | tightly"
    echo "  --concurrency  Concurrent access: 1 | 10 | 100 | 1000 (default: 1)"
    echo "  --cache        Cache: enabled | disabled (default: disabled)"
    echo "  --iterations   Number of iterations for each action (default: 100)"
    echo "  --help         Show this help"
    echo
    echo "Example: $0 --db medium --rel tightly --concurrency 10 --cache enabled"
    exit 1
}

parse_args()  {
    while [[ $# -gt 0 ]]; do
    case "$1" in
        --db)          DB="$2"; shift 2 ;;
        --rel)         REL="$2"; shift 2 ;;
        --concurrency) CONCURRENCY="$2"; shift 2 ;;
        --cache)       CACHE="$2"; shift 2 ;;
        --iterations)  ITERATIONS="$2"; shift 2 ;;
        --help)        usage ;;
        *) echo "Unknown option: $1"; usage ;;
    esac
    done

    if [[ -z "$DB" || -z "$REL" ]]; then
    echo "Missing required parameters."
    usage
    fi
}

map_metta_params() {
    local db_size="$1"
    local rel="$2"

    case "$db_size" in
    empty)
        SENTENCES=2
        WORD_COUNT=2
        WORD_LENGTH=2
        ;;
    small)
        SENTENCES=10000
        WORD_COUNT=5
        WORD_LENGTH=5
        ;;
    medium)
        SENTENCES=100000
        WORD_COUNT=10
        WORD_LENGTH=10
        ;;
    large)
        SENTENCES=1000000
        WORD_COUNT=15
        WORD_LENGTH=15
        ;;
    xlarge)
        SENTENCES=10000000
        WORD_COUNT=20
        WORD_LENGTH=20
        ;;
    *)
        echo "Invalid --db value: $db_size" >&2
        exit 1
        ;;
    esac

    case "$rel" in
    loosely)
        ALPHABET_RANGE="0-10"
        ;;
    tightly)
        ALPHABET_RANGE="0-25"
        ;;
    none)
        ALPHABET_RANGE="0-5"
        ;;
    *)
        echo "Invalid --rel value: $rel" >&2
        exit 1
        ;;
    esac
}

generate_metta_file() {
    local sentence_count=$1
    local word_count=$2
    local word_length=$3
    local alphabet_range=$4

    filename="$METTA_PATH"

    python3 ./src/scripts/python/metta_file_generator.py \
        --filename "$filename" \
        --sentence-count "$sentence_count" \
        --word-count "$word_count" \
        --word-length "$word_length" \
        --alphabet-range "$alphabet_range"

    if [ $? -ne 0 ]; then
        echo "Error generating MeTTa file." >&2
        return 1
    fi

    echo "MeTTa file generated at: $filename"
}

init_environment() {
    # Redis and MongoDB
    DAS_REDIS_HOSTNAME="localhost"
    DAS_REDIS_PORT="29000"
    DAS_USE_REDIS_CLUSTER="false"
    DAS_MONGODB_HOSTNAME="localhost"
    DAS_MONGODB_PORT="28000"
    DAS_MONGODB_USERNAME="dbadmin"
    DAS_MONGODB_PASSWORD="dassecret"
    echo -e "$DAS_REDIS_PORT\nN\n$DAS_MONGODB_PORT\n$DAS_MONGODB_USERNAME\n$DAS_MONGODB_PASSWORD\nN\n8888\n\n\n\n\n" | das-cli config set > /dev/null
    # das-cli db stop > /dev/null
    # das-cli db start
    # das-cli metta load "$METTA_PATH"
    # das-cli attention-broker start

    # # MORK
    # make run-mork-server > /dev/null 2>&1 &  
    # until curl --silent http://localhost:8000/status/-; do
    #     echo "Waiting MORK..."
    #     sleep 5
    # done
    # make mork-loader FILE="$METTA_PATH"
}

load_scenario_definition() {
    filter=".scenarios[] | select(.db == \"$DB\" and .rel == \"$REL\" and .concurrency == $CONCURRENCY and .cache == \"$CACHE\")"

    SCENARIO_YAML=$(yq eval -- "$filter" ./src/tests/benchmark/scenarios.yaml)

    if [[ -z "$SCENARIO_YAML" ]]; then
        echo "Scenario not found for:"
        echo "  db=$DB, rel=$REL, concurrency=$CONCURRENCY, cache=$CACHE"
        exit 1
    fi

    SCENARIO_NAME=$(echo "$SCENARIO_YAML" | yq eval '.name' -)

    mapfile -t ACTIONS < <(printf '%s' "$SCENARIO_YAML" | yq eval '.actions[].name' -)
}

print_scenario() {
    echo "=== Scenario: $SCENARIO_NAME ==="
    echo "db=$DB, rel=$REL, concurrency=$CONCURRENCY, cache=$CACHE"
    echo "actions=${ACTIONS[*]}"
}

run_tests() {
    for action in "${ACTIONS[@]}"; do
        echo "--- Running: $action ---"

        filter_metrics=".actions[] | select(.name == \"$action\") | .metrics[]"

        METRICS=$(
            printf '%s' "$SCENARIO_YAML" \
            | yq eval "$filter_metrics" - \
            | paste -sd',' -
        )

        echo "Running benchmark for action: $action"
        echo "metrics: $METRICS"
        # ./src/scripts/bazel.sh clean --expunge
        sleep 1
        ./src/scripts/bazel.sh run //tests/benchmark:atomdb_benchmark -- "$action" "$CACHE" "$CONCURRENCY" "$ITERATIONS" "$METRICS"

        # output=$(make bazel run //tests/benchmark:atomdb_benchmark -- "$action" "$CACHE" "$CONCURRENCY" "$ITERATIONS" "$METRICS")
        # ret=$?
        # echo "return_INNER" $ret
        # if [[ $ret -ne 0 ]]; then
        #     echo "Error running benchmark for action: $action" >&2
        #     echo "$output" >&2
        #     exit $ret
        # else
        #     echo "Benchmark for action: $action completed successfully."
        #     echo "$output"
        #     return 0
        # fi
    done
}

mainBeautiful() {
    parse_args "$@"

    map_metta_params "$DB" "$REL"

    echo -en "${YELLOW}Setting the the test scenario...${RESET}"
    load_scenario_definition
    ret=$?
    if [[ $ret -eq 0 ]]; then
        echo -e "\r\033[K${GREEN}Setting the the test scenario...OK${RESET}"
        print_scenario
    else
        echo -e "\r\033[K${RED}Setting the the test scenario...FAILED (exit code $ret)${RESET}"
        exit $ret
    fi

    echo -en "${YELLOW}Generating MeTTa file...${RESET}"
    output=$(generate_metta_file "$SENTENCES" "$WORD_COUNT" "$WORD_LENGTH" "$ALPHABET_RANGE" 2>&1)
    ret=$?
    if [[ $ret -eq 0 ]]; then
        echo -e "\r\033[K${GREEN}Generating MeTTa file...OK${RESET}"
        echo "$output"
    else
        echo "\r\033[K${RED}Generating MeTTa file...FAILED (exit code $ret)${RESET}"
        echo "$output" >&2
        exit $ret
    fi

    echo -en "${YELLOW}Initializing the environment...${RESET}"
    output=$(init_environment 2>&1)
    ret=$?
    if [[ $ret -eq 0 ]]; then
        echo -e "\r\033[K${GREEN}Initializing the environment...OK${RESET}"
        echo "$output"
    else
        echo -e "\r\033[K${RED}Initializing the environment...FAILED (exit code $ret)${RESET}"
        echo "$output" >&2
        exit $ret
    fi

    echo -en "${YELLOW}Running benchmark tests...${RESET}"
    run_benchmark
    ret=$?
    echo "return_CMD $ret"
    if [[ $ret -eq 0 ]]; then
        echo -e "\r\033[K${GREEN}Running benchmark tests...OK${RESET}"
        echo "$output"
        echo $ret
    else
        echo -e "\r\033[K${RED}Running benchmark tests...FAILED (exit code $ret)${RESET}"
        echo "$output" >&2
        echo $ret
    fi

}

main() {
    parse_args "$@"
    map_metta_params "$DB" "$REL"
    load_scenario_definition
    print_scenario
    generate_metta_file "$SENTENCES" "$WORD_COUNT" "$WORD_LENGTH" "$ALPHABET_RANGE"
    init_environment
    run_tests
}

main "$@"
