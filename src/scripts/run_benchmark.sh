#!/bin/bash

if [ -z "$1" ]
then
    echo "Usage: run_benchmark.sh BENCHMARK"
    echo "Available benchmarks: atomdb, query_agent"
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
CACHE_ENABLED="false"
ITERATIONS=100
UPDATE_ATTENTION_BROKER="false"
POSITIVE_IMPORTANCE="false"
UNIQUE_ASSIGNMENT="false"
COUNT="false"

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
    query_agent) ;;
    *) echo -e "${RED}Unknown benchmark: $BENCHMARK. Choose either atomdb or query_agent${RESET}"; exit 1 ;;
esac

echo ""
echo -e "${GREEN}** Running benchmark: $BENCHMARK **${RESET}"
echo ""


update_attention_broker=false
positive_importance=false
unique_assignment=false
count=false

usage() {
    echo "Usage: $0 <benchmark> [OPTIONS]"
    echo
    echo "  --db                        Database size: empty | small | medium | large | xlarge (defalut: empty)"
    echo "  --rel                       Atoms relationships: loosely | tightly (default: loosely)"
    echo "  --concurrency               Concurrent access: 1 | 10 | 100 | 1000 (default: 1)"
    echo "  --cache_enabled             Cache enabled: true | false (default: false)"
    echo "  --iterations                Number of iterations for each action (default: 100)"
    echo "  --help                      Show this help"
    echo
    echo "Example: $0 atomdb --db medium --rel tightly --concurrency 1 --cache_enabled true --iterations 100 --update_attention_broker true --positive_importance false --unique_assignment true --count false"
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
        --cache_enabled) CACHE_ENABLED="$2"; shift 2 ;;
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
        SENTENCES=1000
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
    local log_file="$1"

    ./src/scripts/docker_image_build.sh  > /dev/null 2>&1

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
    das-cli db start
    das-cli metta load "$METTA_PATH"
    
    # Attention Broker
    das-cli attention-broker stop > /dev/null
    das-cli attention-broker start 
    echo -e "\r\033[K${GREEN}Redis, MongoDB and Attention Broker initialization completed!${RESET}"
    
    # Query Agent
    if [[ "$BENCHMARK" == "query_agent" ]]; then
        if [[ $(docker ps -q --filter "name=^das-query_broker-") ]]; then
            docker stop $(docker ps -q --filter "name=^das-query_broker-")
        fi
        cat > .env <<EOF
DAS_MONGODB_HOSTNAME=localhost
DAS_MONGODB_PORT=28000
DAS_MONGODB_USERNAME=dbadmin
DAS_MONGODB_PASSWORD=dassecret
DAS_REDIS_HOSTNAME=localhost
DAS_REDIS_PORT=29000
DAS_USE_REDIS_CLUSTER=false
DAS_ATTENTION_BROKER_ADDRESS=localhost
DAS_ATTENTION_BROKER_PORT=37007
EOF
        ./src/scripts/build.sh --copt=-DLOG_LEVEL=DEBUG_LEVEL
        ./src/scripts/run.sh query_broker 35700 3000:3100 | stdbuf -oL grep "Benchmark::" > "$log_file" || true &
        echo -e "\r\033[K${GREEN}Query Agent initialization completed!${RESET}"
    fi

    # MORK
    if [[ $(docker ps -q --filter "name=^das-mork-server-") ]]; then
        docker rm -f $(docker ps -q --filter "name=^das-mork-server-")
    fi
    ./src/scripts/docker_image_build_mork.sh > /dev/null  2> >(grep -v '^+')
    ./src/scripts/mork_server.sh > /dev/null 2>&1 &  
    until curl --silent http://localhost:8000/status/-; do
        echo "Waiting MORK..."
        sleep 5
    done
    ./src/scripts/mork_loader.sh "$METTA_PATH"
    echo -e "\r\033[K${GREEN}MORK initialization completed!${RESET}"
}

load_scenario_definition() {
    local benchmark=$1

    local ini_file="./src/tests/benchmark/$benchmark/scenarios.ini"
    local section=""
    local found_db="" found_rel="" found_conc="" found_cache_enabled="" found_actions=""

    while IFS= read -r line; do
        if [[ $line =~ ^\[(.+)\]$ ]]; then
            section="${BASH_REMATCH[1]}"
            found_db=""
            found_rel=""
            found_conc=""
            found_cache_enabled=""
            found_actions=""
        elif [[ $line =~ ^db=(.*) ]]; then
            found_db="${BASH_REMATCH[1]}"
        elif [[ $line =~ ^rel=(.*) ]]; then
            found_rel="${BASH_REMATCH[1]}"
        elif [[ $line =~ ^concurrency=(.*) ]]; then
            found_conc="${BASH_REMATCH[1]}"
        elif [[ $line =~ ^cache_enabled=(.*) ]]; then
            found_cache_enabled="${BASH_REMATCH[1]}"
        elif [[ $line =~ ^actions=(.*) ]]; then
            found_actions="${BASH_REMATCH[1]}"
        fi

        if [[ -n "$section" && \
            "$found_db" == "$DB" && \
            "$found_rel" == "$REL" && \
            "$found_conc" == "$CONCURRENCY" && \
            "$found_cache_enabled" == "$CACHE_ENABLED" && \
            -n "$found_actions" ]]; then
            SCENARIO_NAME="$section"
            IFS=',' read -ra ACTIONS <<< "$found_actions"
            return 0
        fi
    done < "$ini_file"

    echo ""
    echo "Scenario not found for:"
    echo "  db=$DB, rel=$REL, concurrency=$CONCURRENCY, cache_enabled=$CACHE_ENABLED"
    exit 1
}

print_scenario() {
    echo ""
    echo "=== Scenario: $SCENARIO_NAME ==="
    echo "db=$DB, rel=$REL, concurrency=$CONCURRENCY, cache_enabled=$CACHE_ENABLED"
    echo "actions=${ACTIONS[*]}"
    echo ""
}

run_benchmark() {
    local benchmark=$1
    local report_base_directory="/tmp/${benchmark}_benchmark/${TIMESTAMP}"

    mkdir -p "$report_base_directory"

    ATOMDB_TYPES=("redismongodb" "morkdb")

    if [[ "$benchmark" == "query_agent" ]]; then
        for type in "${ATOMDB_TYPES[@]}"; do   
            for action in "${ACTIONS[@]}"; do
                echo -e "\n== Running benchmarks for Query Agent: $type | Action: $action =="
                local log_file="/tmp/${BENCHMARK}_benchmark/${TIMESTAMP}/${type}_${action}_server_log.txt"
                local client_log_file="/tmp/${BENCHMARK}_benchmark/${TIMESTAMP}/${type}_${action}_client_log.txt"
                init_environment "$log_file"
                echo -e "\r\033[K${GREEN}Running test START${RESET}"
                echo -e "\r\033[K${GREEN}Partial results in /tmp/${BENCHMARK}_benchmark/${TIMESTAMP}/${RESET}"
                stdbuf -oL ./src/scripts/bazel.sh run //tests/benchmark/query_agent:query_agent_main --copt=-DLOG_LEVEL=DEBUG_LEVEL -- "$report_base_directory" "$type" "$action" "$CACHE_ENABLED" "$ITERATIONS" "/tmp/${BENCHMARK}_benchmark/${TIMESTAMP}/${type}_${action}_" | tee "$client_log_file"
                echo -e "\r\033[K${GREEN}Running test END${RESET}"
            done
        done
    elif [[ "$benchmark" == "atomdb" ]]; then
        # --- Set environment for each method of each action of each atomdb
        AddAtom=("add_node" "add_link" "add_atom_node" "add_atom_link")
        AddAtoms=("add_nodes" "add_links" "add_atoms_node" "add_atoms_link")
        GetAtom=("get_node_document" "get_link_document" "get_atom_document_node" "get_atom_document_link" "get_atom_node" "get_atom_link")
        GetAtoms=("get_node_documents" "get_link_documents" "get_atom_documents_node" "get_atom_documents_link" "query_for_pattern" "query_for_targets")
        DeleteAtom=("delete_node" "delete_link" "delete_atom_node" "delete_atom_link")
        DeleteAtoms=("delete_nodes" "delete_links" "delete_atoms_node" "delete_atoms_link")

        for type in "${ATOMDB_TYPES[@]}"; do
            for action in "${ACTIONS[@]}"; do
                declare -n methods="$action"
                for method in "${methods[@]}"; do
                    echo -e "\n== Running benchmarks for AtomDB: $type | Action: $action | Method: $method =="
                    init_environment "$type"
                    ./src/scripts/bazel.sh run //tests/benchmark/atomdb:atomdb_main --copt=-DLOG_LEVEL=DEBUG_LEVEL -- "$type" "$action" "$method" "$CACHE_ENABLED" "$CONCURRENCY" "$ITERATIONS" "$TIMESTAMP"
                done
            done
        done
    fi
}

scenario_data() {
    echo "$SCENARIO_NAME $DB $REL $CONCURRENCY $CACHE_ENABLED $ITERATIONS"
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