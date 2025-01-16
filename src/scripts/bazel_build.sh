#!/bin/bash -x

(( JOBS=$(nproc)/2 ))
BAZELISK_CMD=/opt/bazel/bazelisk
REPO_ROOT=/opt/das-attention-broker
WORKSPACE_DIR=${REPO_ROOT}/cpp
BIN_DIR=${REPO_ROOT}/bin
mkdir -p $BIN_DIR

cd $WORKSPACE_DIR \
&& $BAZELISK_CMD build --jobs $JOBS --enable_bzlmod //:link_creation_engine \
&& mv bazel-bin/link_creation_engine $BIN_DIR \
&& $BAZELISK_CMD build --jobs $JOBS --enable_bzlmod //:word_query \
&& mv bazel-bin/word_query $BIN_DIR \
&& $BAZELISK_CMD build --jobs $JOBS --enable_bzlmod //:attention_broker_service \
&& mv bazel-bin/attention_broker_service $BIN_DIR \
&& $BAZELISK_CMD build --jobs $JOBS --enable_bzlmod //:query_broker \
&& mv bazel-bin/query_broker $BIN_DIR \
&& $BAZELISK_CMD build --jobs $JOBS --enable_bzlmod //:query \
&& mv bazel-bin/query $BIN_DIR \
&& chown -R ${_USER}:${_GROUP} ${BIN_DIR}


