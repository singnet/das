#!/bin/bash -x

set -eoux pipefail

BAZELISK_CMD=/opt/bazel/bazelisk
BAZELISK_BUILD_CMD="${BAZELISK_CMD} build --noshow_progress"
[ "${BAZEL_JOBS:-x}" != "x" ] && BAZELISK_BUILD_CMD="${BAZELISK_BUILD_CMD} --jobs=${BAZEL_JOBS}"
BAZELISK_RUN_CMD="${BAZELISK_CMD} run"
BUILD_TARGETS=""
MOVE_TARGETS=""

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
    BUILD_TARGETS+=" //:inference_agent_server"
    BUILD_TARGETS+=" //:inference_agent_client"
    BUILD_TARGETS+=" //:link_creation_server"
    BUILD_TARGETS+=" //:link_creation_agent_client"
    BUILD_TARGETS+=" //:word_query"
    BUILD_TARGETS+=" //:word_query_evolution"
    BUILD_TARGETS+=" //:attention_broker_service"
    BUILD_TARGETS+=" //:query_broker"
    BUILD_TARGETS+=" //:evolution_broker"
    BUILD_TARGETS+=" //:evolution_client"
    BUILD_TARGETS+=" //:query"
    BUILD_TARGETS+=" //:das"

    MOVE_TARGETS+=" bazel-bin/inference_agent_server"
    MOVE_TARGETS+=" bazel-bin/inference_agent_client"
    MOVE_TARGETS+=" bazel-bin/link_creation_server"
    MOVE_TARGETS+=" bazel-bin/link_creation_agent_client"
    MOVE_TARGETS+=" bazel-bin/word_query"
    MOVE_TARGETS+=" bazel-bin/word_query_evolution"
    MOVE_TARGETS+=" bazel-bin/attention_broker_service"
    MOVE_TARGETS+=" bazel-bin/query_broker"
    MOVE_TARGETS+=" bazel-bin/evolution_broker"
    MOVE_TARGETS+=" bazel-bin/evolution_client"
    MOVE_TARGETS+=" bazel-bin/query"
    MOVE_TARGETS+=" bazel-bin/das.so"

fi

if [ "$BUILD_WHEELS" = true ]; then
    $BAZELISK_RUN_CMD //deps:requirements_python_client.update
fi

$BAZELISK_BUILD_CMD $BUILD_TARGETS
mv $MOVE_TARGETS $BIN_DIR

exit $?
