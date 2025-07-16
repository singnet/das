#!/usr/bin/env bash

set -eou pipefail

ATOMDB_TYPES=("redis_mongo" "mork")

TIMESTAMP=$(date +%Y%m%d%H%M%S)

DB=""
REL="none"
CONCURRENCY="1"
CACHE="disabled"
ITERATIONS=100

SENTENCES=""
WORD_COUNT=""
WORD_LENGTH=""
ALPHABET_RANGE=""
METTA_PATH="/tmp/${RANDOM}_${TIMESTAMP}.metta"

RESET='\033[0m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
GREEN='\033[0;32m'


usage() {
    echo "Usage: $0 [OPTIONS]"
    echo
    echo "  --db           Database size: empty | small | medium | large | xlarge"
    echo "  --rel          Atoms relationships: loosely | tightly (default: none)"
    echo "  --concurrency  Concurrent access: 1 | 10 | 100 | 1000 (default: 1)"
    echo "  --cache        Cache: enabled | disabled (default: disabled)"
    echo "  --iterations   Number of iterations for each action (default: 100)"
    echo "  --help         Show this help"
    echo
    echo "Example: $0 --db medium --rel tightly --concurrency 10 --cache enabled --iterations 100"
    exit 1
}

check_dependencies() {
    local missing=0

    # Check das-cli
    if ! command -v das-cli &>/dev/null; then
        echo "Error: 'das-cli' is not installed or not found in your PATH."
        missing=1
    fi

    # Check Python 3
    if command -v python3 &>/dev/null; then
        PYTHON_CMD="python3"
    elif command -v python &>/dev/null && python --version 2>&1 | grep -q "Python 3"; then
        PYTHON_CMD="python"
    else
        echo "Error: Python 3 is not installed or not found in your PATH."
        missing=1
    fi

    if [[ $missing -eq 1 ]]; then
        echo "Please install the missing dependencies and try again."
        exit 1
    fi
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

    if [[ -z "$DB" ]]; then
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
        SENTENCES=100000
        WORD_COUNT=5
        WORD_LENGTH=5
        ;;
    medium)
        SENTENCES=1000000
        WORD_COUNT=10
        WORD_LENGTH=10
        ;;
    large)
        SENTENCES=10000000
        WORD_COUNT=15
        WORD_LENGTH=15
        ;;
    xlarge)
        SENTENCES=100000000
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
        ALPHABET_RANGE="0-2"
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

    echo "The MeTTa file was generated at: $filename"
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
    das-cli db stop > /dev/null
    das-cli attention-broker stop > /dev/null
    das-cli db start
    das-cli metta load "$METTA_PATH"
    TOTAL_ATOMS=$(das-cli db count-atoms) > /dev/null 2>&1
    das-cli attention-broker start 
    echo -e "\r\033[K${GREEN}Redis, MongoDB and Attention Broker initialization completed!${RESET}"
    # MORK
    ./src/scripts/docker_image_build_mork.sh > /dev/null  2> >(grep -v '^+')
    ./src/scripts/mork_server.sh > /dev/null 2>&1 &  
    until curl --silent http://localhost:8000/status/-; do
        echo "Waiting MORK..."
        sleep 5
    done
    src/scripts/mork_loader.sh "$METTA_PATH" 2> >(grep -v '^+')
    echo -e "\r\033[K${GREEN}MORK initialization completed!${RESET}"
}

load_scenario_definition() {
    local ini_file="./src/tests/benchmark/scenarios.ini"
    local section=""
    local found_db="" found_rel="" found_conc="" found_cache="" found_actions=""

    while IFS= read -r line; do
        if [[ $line =~ ^\[(.+)\]$ ]]; then
            section="${BASH_REMATCH[1]}"
            found_db=""
            found_rel=""
            found_conc=""
            found_cache=""
            found_actions=""
        elif [[ $line =~ ^db=(.*) ]]; then
            found_db="${BASH_REMATCH[1]}"
        elif [[ $line =~ ^rel=(.*) ]]; then
            found_rel="${BASH_REMATCH[1]}"
        elif [[ $line =~ ^concurrency=(.*) ]]; then
            found_conc="${BASH_REMATCH[1]}"
        elif [[ $line =~ ^cache=(.*) ]]; then
            found_cache="${BASH_REMATCH[1]}"
        elif [[ $line =~ ^actions=(.*) ]]; then
            found_actions="${BASH_REMATCH[1]}"
        fi

        if [[ -n "$section" && \
              "$found_db" == "$DB" && \
              "$found_rel" == "$REL" && \
              "$found_conc" == "$CONCURRENCY" && \
              "$found_cache" == "$CACHE" && \
              -n "$found_actions" ]]; then
            SCENARIO_NAME="$section"
            IFS=',' read -ra ACTIONS <<< "$found_actions"
            return 0
        fi
    done < "$ini_file"

    echo "Scenario not found for:"
    echo "  db=$DB, rel=$REL, concurrency=$CONCURRENCY, cache=$CACHE"
    exit 1
}

print_scenario() {
    echo "=== Scenario: $SCENARIO_NAME ==="
    echo "db=$DB, rel=$REL, concurrency=$CONCURRENCY, cache=$CACHE, iterations=$ITERATIONS"
    echo "actions=${ACTIONS[*]}"
    echo ""
}

run_benchmark() {
    mkdir -p "/tmp/atomdb_benchmark/${TIMESTAMP}"

    for type in "${ATOMDB_TYPES[@]}"; do
        echo -e "\n== Running benchmarks for AtomDB type: $type =="
        init_environment    
        for action in "${ACTIONS[@]}"; do
            echo -e "${YELLOW}>> Running action: $action <<${RESET}"
            ./src/scripts/bazel.sh run //tests/benchmark:atomdb_benchmark -- "$type" "$action" "$CACHE" "$CONCURRENCY" "$ITERATIONS" "$TIMESTAMP" 2> >(grep -v '^+')
        done
        echo -e "\r\033[K${GREEN}Benchmark for AtomDB $type completed!${RESET}"
    done
}

header_to_report() {
    local header="Consolidated AtomDB Benchmark Report
=========================================================

## Test environment
 -CPU = $(lscpu | grep 'Model name' | sed 's/Model name:[ \t]*//') ($(lscpu | awk '/^CPU\(s\):/ {print $2}' | head -1) Cores)
 -Free Memory = $(free -g | awk '/^Mem:/ {print $2 " GB"}')
 -Disk = $(df -h --output=size / | tail -1 | tr -d ' G') GB
 -SO = $(lsb_release -a | grep Description | sed 's/Description:[ \t]*//')

## Test components
 -Database = $DB
 -Atoms relationships = $REL
 -Concurrent access = $CONCURRENCY
 -Cache = $CACHE
 -iterations = $ITERATIONS
 -Total Atoms in database: $TOTAL_ATOMS

## Legend
 -AVG  = Average Operation Time (ms)
 -MIN  = Minimum Operation Time (ms)
 -MAX  = Maximum Operation Time (ms)
 -P50  = 50th Percentile Time (ms)
 -P90  = 90th Percentile Time (ms)
 -P99  = 99th Percentile Time (ms)
 -TT   = Total Time (ms)
 -TPA  = Time per Atom (ms)
 -TP   = Throughput (atoms/sec)

=========================================================
"
    echo "$header"
}


consolidate_reports() {
    OUTPUT_DIR="/tmp/atomdb_benchmark/${TIMESTAMP}/consolidated_report_scenario_${SCENARIO_NAME}.txt"
    python3 ./src/scripts/python/consodidate_atomdb_benchmark.py "/tmp/atomdb_benchmark/${TIMESTAMP}" --output "$OUTPUT_DIR" --header "$(header_to_report)"
    echo ""
    echo -e "\r\033[K${GREEN}Consolidated reports saved to: $OUTPUT_DIR${RESET}"
}

main() {
    parse_args "$@"
    check_dependencies
    map_metta_params "$DB" "$REL"
    load_scenario_definition
    print_scenario
    generate_metta_file "$SENTENCES" "$WORD_COUNT" "$WORD_LENGTH" "$ALPHABET_RANGE"
    run_benchmark
    consolidate_reports
}

main "$@"