#!/bin/bash -x

set -eoux pipefail

BAZELISK_CMD=/opt/bazel/bazelisk
BAZELISK_BUILD_CMD="${BAZELISK_CMD} build"
BAZELISK_RUN_CMD="${BAZELISK_CMD} run"

BUILD_BINARIES=false
BUILD_WHEELS=false

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

if [ "$BUILD_BINARIES" = false ] && [ "$BUILD_WHEELS" = false ]; then
    BUILD_BINARIES=true
    BUILD_WHEELS=true
fi

if [ "$BUILD_BINARIES" = true ]; then
    $BAZELISK_BUILD_CMD \
      //:inference_agent_server \
      //:inference_agent_client \
      //:link_creation_server \
      //:link_creation_agent_client \
      //:word_query \
      //:attention_broker_service \
      //:query_broker \
      //:query

    mv bazel-bin/inference_agent_server "$BIN_DIR"
    mv bazel-bin/inference_agent_client "$BIN_DIR"
    mv bazel-bin/link_creation_server "$BIN_DIR"
    mv bazel-bin/link_creation_agent_client "$BIN_DIR"
    mv bazel-bin/word_query "$BIN_DIR"
    mv bazel-bin/attention_broker_service "$BIN_DIR"
    mv bazel-bin/query_broker "$BIN_DIR"
    mv bazel-bin/query "$BIN_DIR"

fi

if [ "$BUILD_WHEELS" = true ]; then
    $BAZELISK_RUN_CMD //deps:requirements.update
    $BAZELISK_RUN_CMD //deps:requirements_dev.update

    $BAZELISK_BUILD_CMD \
      //hyperon_das_atomdb:hyperon_das_atomdb_wheel \
      //hyperon_das:hyperon_das_wheel \
      //hyperon_das_node:hyperon_das_node_wheel \
      //hyperon_das_atomdb_cpp:hyperon_das_atomdb_cpp_wheel

    mv bazel-bin/hyperon_das_atomdb/*.whl "$BIN_DIR"
    mv bazel-bin/hyperon_das/*.whl "$BIN_DIR"
    mv bazel-bin/hyperon_das_node/*.whl "$BIN_DIR"
    mv bazel-bin/hyperon_das_atomdb_cpp/*.whl "$BIN_DIR"
fi

exit $?
