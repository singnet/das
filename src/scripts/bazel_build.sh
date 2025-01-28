#!/bin/bash -x

(( JOBS=$(nproc)/2 ))
BAZELISK_CMD=/opt/bazel/bazelisk
BAZELISK_BUILD_CMD="${BAZELISK_CMD} build --jobs ${JOBS} --enable_bzlmod"

# CPP build
cd $CPP_WORKSPACE_DIR \
&& $BAZELISK_BUILD_CMD //:link_creation_server \
&& mv bazel-bin/link_creation_server $BIN_DIR \
&& $BAZELISK_BUILD_CMD //:word_query \
&& mv bazel-bin/word_query $BIN_DIR \
&& $BAZELISK_BUILD_CMD //:attention_broker_service \
&& mv bazel-bin/attention_broker_service $BIN_DIR \
&& $BAZELISK_BUILD_CMD //:query_broker \
&& mv bazel-bin/query_broker $BIN_DIR \
&& $BAZELISK_BUILD_CMD //:query \
&& mv bazel-bin/query $BIN_DIR

# Python build
cd $PYTHON_WORKSPACE_DIR \
&& $BAZELISK_BUILD_CMD //deps:requirements.update \
&& $BAZELISK_BUILD_CMD //deps:requirements_dev.update \
&& $BAZELISK_BUILD_CMD //hyperon_das_atomdb:hyperon_das_atomdb_wheel --define=ATOMDB_VERSION=0.8.11 \
&& mv bazel-bin/hyperon_das_atomdb_wheel $BIN_DIR \
&& $BAZELISK_BUILD_CMD //hyperon_das:hyperon_das_wheel --define=DAS_VERSION=0.9.17 \
&& mv bazel-bin/hyperon_das_atomdb_wheel $BIN_DIR
