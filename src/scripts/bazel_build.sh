#!/bin/bash -x

(( JOBS=$(nproc)/2 ))
BAZELISK_CMD=/opt/bazel/bazelisk
BAZELISK_BUILD_CMD="${BAZELISK_CMD} build --jobs ${JOBS} --enable_bzlmod"

cd $WORKSPACE_DIR \
&& $BAZELISK_BUILD_CMD //:link_creation_server \
&& mv bazel-bin/link_creation_server $BIN_DIR \
&& $BAZELISK_BUILD_CMD //:link_creation_engine \
&& mv bazel-bin/link_creation_engine $BIN_DIR \
&& $BAZELISK_BUILD_CMD //:word_query \
&& mv bazel-bin/word_query $BIN_DIR \
&& $BAZELISK_BUILD_CMD //:attention_broker_service \
&& mv bazel-bin/attention_broker_service $BIN_DIR \
&& $BAZELISK_BUILD_CMD //:query_broker \
&& mv bazel-bin/query_broker $BIN_DIR \
&& $BAZELISK_BUILD_CMD //:query \
&& mv bazel-bin/query $BIN_DIR 
