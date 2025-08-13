#!/bin/bash

if [ -z "$1" ]
then
    echo "Usage: run_benchmark.sh BENCHMARK"
    echo "Available benchmarks: atomdb, pattern_matching_query"
    exit 1
else
    BENCHMARK="${1}"
    shift
fi

set -eou pipefail

TIMESTAMP=$(date +%Y%m%d%H%M%S)

DB="empty"
REL="loosely"
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

case "$BENCHMARK" in
    atomdb) ;;
    pattern_matching_query) ;;
    *) echo -e "${RED}Unknown benchmark: $BENCHMARK. Choose either atomdb or pattern_matching_query${RESET}"; exit 1 ;;
esac

echo ""
echo -e "${GREEN}** Running benchmark: $BENCHMARK **${RESET}"
echo ""

usage() {
    echo "Usage: $0 <benchmark> [OPTIONS]"
    echo
    echo "  --db           Database size: empty | small | medium | large | xlarge (defalut: empty)"
    echo "  --rel          Atoms relationships: loosely | tightly (default: loosely)"
    echo "  --concurrency  Concurrent access: 1 | 10 | 100 | 1000 (default: 1)"
    echo "  --cache        Cache: enabled | disabled (default: disabled)"
    echo "  --iterations   Number of iterations for each action (default: 100)"
    echo "  --help         Show this help"
    echo
    echo "Example: $0 atomdb --db medium --rel tightly --concurrency 10 --cache enabled --iterations 100"
    exit 1
}

check_dependencies() {
    local missing=0

    # Check das-cli
    if ! command -v das-cli &>/dev/null; then
        echo "Error: 'das-cli' is not installed or not found in your PATH."
        missing=1
    fi

    if ! das-cli --version | grep -q "0.4.14"; then
        echo -e "\r\033[K${YELLOW}Warning: das-cli version 0.4.14 is required for this script.${RESET}"
        echo "Please install das-cli version 0.4.14."
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

    # Docker with buildx
    if ! command -v docker buildx &>/dev/null; then
        echo "Error: 'docker buildx' is not installed or not found in your PATH."
        echo "Please install Docker with buildx and ensure it is running."
        exit 1
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

set_metta_file_params() {
    local db_size="$1"
    local rel="$2"

    case "$db_size" in
    empty)
        SENTENCES=100
        ;;
    small)
        SENTENCES=10000
        ;;
    medium)
        SENTENCES=1000000
        ;;
    large)
        SENTENCES=10000000
        ;;
    xlarge)
        SENTENCES=100000000
        ;;
    *)
        echo "Invalid --db value: $db_size" >&2
        exit 1
        ;;
    esac

    # Smaller vocabulary, greater connectivity (V = A^L)
    case "$rel" in
    loosely)
        ALPHABET_RANGE="0-25"
        WORD_LENGTH=3
        WORD_COUNT=5
        ;;
    tightly)
        ALPHABET_RANGE="0-5"
        WORD_LENGTH=3
        WORD_COUNT=10
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
    echo -e "$DAS_REDIS_PORT\nN\n$DAS_MONGODB_PORT\n$DAS_MONGODB_USERNAME\n$DAS_MONGODB_PASSWORD\nN\n8888\n\n\n\n\n\n\n\n\n\n\n\n" | das-cli config set > /dev/null
    das-cli db stop > /dev/null
    das-cli attention-broker stop > /dev/null
    das-cli db start
    das-cli metta load "$METTA_PATH"
    TOTAL_ATOMS=$(das-cli db count-atoms) > /dev/null 2>&1
    das-cli attention-broker start 
    echo -e "\r\033[K${GREEN}Redis, MongoDB and Attention Broker initialization completed!${RESET}"
    
    # MORK
    mork_server_containers=$(docker ps -a --filter "name=das-mork-server" --format "{{.ID}}")
    if [[ -n "$mork_server_containers" ]]; then
        docker rm -f $mork_server_containers > /dev/null 2>&1
    fi
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
    local benchmark=$1

    local ini_file="./src/tests/benchmark/$benchmark/scenarios.ini"
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
    echo ""
    echo "=== Scenario: $SCENARIO_NAME ==="
    echo "db=$DB, rel=$REL, concurrency=$CONCURRENCY, cache=$CACHE, iterations=$ITERATIONS"
    echo "actions=${ACTIONS[*]}"
    echo ""
}

run_benchmark() {
    local benchmark=$1

    mkdir -p "/tmp/${benchmark}_benchmark/${TIMESTAMP}"

    if [[ "$benchmark" == "pattern_matching_query" ]]; then
        echo -e "\r\033[K${YELLOW}Pattern Matching Query benchmark is not implemented yet.${RESET}"
        return 0
    elif [[ "$benchmark" == "atomdb" ]]; then
        # --- Set environment for each method of each action of each atomdb
        AddAtom=("add_node" "add_link" "add_atom_node" "add_atom_link")
        AddAtoms=("add_nodes" "add_links" "add_atoms_node" "add_atoms_link")
        GetAtom=("get_node_document" "get_link_document" "get_atom_document_node" "get_atom_document_link" "get_atom_node" "get_atom_link")
        GetAtoms=("get_node_documents" "get_link_documents" "get_atom_documents_node" "get_atom_documents_link" "query_for_pattern" "query_for_targets")
        DeleteAtom=("delete_node" "delete_link" "delete_atom_node" "delete_atom_link")
        DeleteAtoms=("delete_nodes" "delete_links" "delete_atoms_node" "delete_atoms_link")

        ATOMDB_TYPES=("redis_mongo" "mork")

        for type in "${ATOMDB_TYPES[@]}"; do
            for action in "${ACTIONS[@]}"; do
                declare -n methods="$action"
                for method in "${methods[@]}"; do
                    echo -e "\n== Running benchmarks for AtomDB: $type | Action: $action | Method: $method ==" 
                    init_environment
                    ./src/scripts/docker_image_build.sh  > /dev/null 2>&1
                    ./src/scripts/bazel.sh run //tests/benchmark/atomdb:atomdb_main -- "$type" "$action" "$method" "$CACHE" "$CONCURRENCY" "$ITERATIONS" "$TIMESTAMP" 2> >(grep -v '^+')
                done
            done
        done
    fi
}


header_to_report() {
    local header="Consolidated AtomDB Benchmark Report
=========================================================

## Test environment
 -CPU = $(lscpu | grep 'Model name' | sed 's/Model name:[ \t]*//') ($(lscpu | awk '/^CPU\(s\):/ {print $2}' | head -1) Cores)
 -Free Memory = $(free -g | awk '/^Mem:/ {print $2 " GB"}')
 -Disk = $(df -h --output=size / | tail -1 | tr -d ' G') GB
 -OS = $(lsb_release -a | grep Description | sed 's/Description:[ \t]*//')

## Test components
 -Database = $DB
 -Atoms relationships = $REL
 -Concurrent access = $CONCURRENCY
 -Cache = $CACHE
 -iterations = $ITERATIONS

## Legend
 -MED  = Median Operation Time (ms)
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

scenario_data() {
    echo "$SCENARIO_NAME $DB $REL $CONCURRENCY $CACHE $ITERATIONS"
}


consolidate_reports() {
    local benchmark=$1
    python3 ./src/scripts/python/consolidate_benchmark.py "/tmp/${benchmark}_benchmark/${TIMESTAMP}" --scenario "$(scenario_data)" --type "$benchmark" --output-file "/tmp/${benchmark}_benchmark/${TIMESTAMP}/consolidated_report.txt" --header-text "$(header_to_report)" --db-path "/home/$USER/.cache/das/benchmark.db"
    echo ""
    echo -e "\r\033[K${GREEN}Consolidated reports saved to database${RESET}"
}

main() {
    check_dependencies
    parse_args "$@"
    set_metta_file_params "$DB" "$REL"
    generate_metta_file "$SENTENCES" "$WORD_COUNT" "$WORD_LENGTH" "$ALPHABET_RANGE"
    load_scenario_definition $BENCHMARK
    print_scenario
    run_benchmark $BENCHMARK
    consolidate_reports $BENCHMARK
}

main "$@"