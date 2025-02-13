#!/bin/bash -x

set -eoux pipefail

ln -sfn /opt/bazel/bazelisk /usr/bin/bazel;

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
  bazel build //:link_creation_server
  mv bazel-bin/link_creation_server "$BIN_DIR"
  bazel build //:link_creation_agent_client
  mv bazel-bin/link_creation_agent_client "$BIN_DIR"
  bazel build //:word_query
  mv bazel-bin/word_query "$BIN_DIR"
  bazel build //:attention_broker_service
  mv bazel-bin/attention_broker_service "$BIN_DIR"
  bazel build //:query_broker
  mv bazel-bin/query_broker "$BIN_DIR"
  bazel build //:query
  mv bazel-bin/query "$BIN_DIR"
fi

if [ "$BUILD_WHEELS" = true ]; then
  bazel run //deps:requirements.update
  bazel run //deps:requirements_dev.update
  bazel build //hyperon_das_atomdb:hyperon_das_atomdb_wheel --define=ATOMDB_VERSION=0.8.11
  bazel build //hyperon_das:hyperon_das_wheel --define=DAS_VERSION=0.9.17
  mv bazel-bin/hyperon_das_atomdb/*.whl "$BIN_DIR"
  mv bazel-bin/hyperon_das/*.whl "$BIN_DIR"
  bazel build //hyperon_das_node:hyperon_das_node_wheel --define=DAS_NODE_VERSION=0.0.1
  mv bazel-bin/hyperon_das_node/*.whl "$BIN_DIR"
  bazel build //hyperon_das_atomdb_cpp:hyperon_das_atomdb_cpp_wheel \
    --define=ATOMDB_VERSION=0.8.11
  mv bazel-bin/hyperon_das_atomdb_cpp/*.whl "$BIN_DIR"
fi

exit $?
