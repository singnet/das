#!/bin/bash

set -eou pipefail

DB=""
REL=""
CONCURRENCY=""
CACHE=""
SENTENCES=""
WORD_COUNT=""
WORD_LENGTH=""
ALPHABET_RANGE=""
METTA_PATH="/tmp/${RANDOM}_$(date +%Y%m%d%H%M%S).metta"

usage() {
    echo "Usage: $0 [OPTIONS]"
    echo
    echo "  --db           Database size: empty | small | medium | large | xlarge"
    echo "  --rel          Atoms relationships: loosely | tightly"
    echo "  --concurrency  Concurrent access: 1 | 10 | 100 | 1000"
    echo "  --cache        Cache: enabled | disabled"
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
        --help)        usage ;;
        *) echo "Unknown option: $1"; usage ;;
    esac
    done

    if [[ -z "$DB" || -z "$REL" || -z "$CONCURRENCY" || -z "$CACHE" ]]; then
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

load_database() {
    echo
    echo "Loading database"
    echo

    DAS_REDIS_HOSTNAME="localhost"
    DAS_REDIS_PORT="29000"
    DAS_USE_REDIS_CLUSTER="false"
    DAS_MONGODB_HOSTNAME="localhost"
    DAS_MONGODB_PORT="28000"
    DAS_MONGODB_USERNAME="dbadmin"
    DAS_MONGODB_PASSWORD="dassecret"
    echo -e "$DAS_REDIS_PORT\nN\n$DAS_MONGODB_PORT\n$DAS_MONGODB_USERNAME\n$DAS_MONGODB_PASSWORD\nN\n8888\n\n\n\n\n" | das-cli config set
    das-cli db start
    das-cli metta load "$METTA_PATH"
    das-cli attention-broker start

    ./src/scripts/mork
}

run_benchmark() {
    # WIP
    echo "Running benchmark"
}

main() {
    parse_args "$@"

    map_metta_params "$DB" "$REL"

    echo -n "Generating MeTTa file…"
    output=$(generate_metta_file "$SENTENCES" "$WORD_COUNT" "$WORD_LENGTH" "$ALPHABET_RANGE" 2>&1)
    ret=$?
    if [[ $ret -eq 0 ]]; then
        echo "OK"
        echo "$output"
    else
        echo "FAILED (exit code $ret)"
        echo "$output" >&2
        exit $ret
    fi

    load_database

}

main "$@"
