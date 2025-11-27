#!/bin/bash

set -eou pipefail 

BAZELISK_CMD=/opt/bazel/bazelisk
BAZELISK_BUILD_CMD="${BAZELISK_CMD} build --noshow_progress --strategy=CppCompile=standalone --spawn_strategy=standalone"
[ "${BAZEL_JOBS:-x}" != "x" ] && BAZELISK_BUILD_CMD="${BAZELISK_BUILD_CMD} --jobs=${BAZEL_JOBS}"
BAZELISK_RUN_CMD="${BAZELISK_CMD} run"
BUILD_TARGETS=""
MOVE_LIB_TARGETS=""
MOVE_BIN_TARGETS=""

for arg in "$@"; do
    case $arg in
        --binaries)
            BUILD_BINARIES=true
            ;;
        --wheels)
            BUILD_WHEELS=true
            ;;
    esac
done

BUILD_BINARIES=${BUILD_BINARIES:-false}
BUILD_WHEELS=${BUILD_WHEELS:-false}

if [ "$BUILD_BINARIES" = false ] && [ "$BUILD_WHEELS" = false ]; then
    BUILD_BINARIES=true
    BUILD_WHEELS=true
fi

if [ "$BUILD_BINARIES" = true ]; then
    # Binaries
    BUILD_TARGETS+=" //:das"
    BUILD_TARGETS+=" //:attention_broker_service"
    BUILD_TARGETS+=" //:attention_broker_client"
    BUILD_TARGETS+=" //:busnode"
    BUILD_TARGETS+=" //:busclient"

    # Other binaries
    BUILD_TARGETS+=" //:word_query"
    BUILD_TARGETS+=" //:word_query_evolution"
    BUILD_TARGETS+=" //:implication_query_evolution"
    BUILD_TARGETS+=" //:tests_db_loader"

    # Move targets
    MOVE_LIB_TARGETS+=" bazel-bin/hyperon_das.so"
    MOVE_BIN_TARGETS+=" bazel-bin/attention_broker_service"
    MOVE_BIN_TARGETS+=" bazel-bin/attention_broker_client"
    MOVE_BIN_TARGETS+=" bazel-bin/busnode"
    MOVE_BIN_TARGETS+=" bazel-bin/busclient"

    # Other binaries
    MOVE_BIN_TARGETS+=" bazel-bin/word_query"
    MOVE_BIN_TARGETS+=" bazel-bin/word_query_evolution"
    MOVE_BIN_TARGETS+=" bazel-bin/implication_query_evolution"
    MOVE_BIN_TARGETS+=" bazel-bin/tests_db_loader"


fi

if [ "$BUILD_WHEELS" = true ]; then
    $BAZELISK_RUN_CMD //deps:requirements_python_client.update
fi

$BAZELISK_BUILD_CMD $BUILD_TARGETS "$@"

mkdir -p $BIN_DIR $LIB_DIR

mv $MOVE_BIN_TARGETS $BIN_DIR
mv $MOVE_LIB_TARGETS $LIB_DIR

find "$LIB_DIR" -type f -name "*.so" | while IFS= read -r sofile; do
    ldd "$sofile" | awk '/=> \// { print $3 }' | while IFS= read -r dep; do
        dep_base=$(basename "$dep")
        case "$dep_base" in
            "libmongoc2.so.2"|"libmongocxx.so._noabi"|"libhiredis.so.1.1.0"|"libhiredis_cluster.so.0.12"|"libbson2.so.2"|"libbsoncxx.so._noabi")
                if [ -f "$dep" ]; then
                    cp "$dep" "$LIB_DIR"
                fi
                ;;
        esac
    done
done


exit $?
