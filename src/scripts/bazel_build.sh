#!/bin/bash -x

(( JOBS=$(nproc)/2 ))
BAZELISK_CMD=/opt/bazel/bazelisk
BIN_FOLDER=/opt/das-attention-broker/bin
mkdir -p $BIN_FOLDER

$BAZELISK_CMD build --jobs $JOBS --noenable_bzlmod //cpp:link_creation_engine \
&& mv bazel-bin/cpp/link_creation_engine $BIN_FOLDER \
&& $BAZELISK_CMD build --jobs $JOBS --noenable_bzlmod //cpp:word_query \
&& mv bazel-bin/cpp/word_query $BIN_FOLDER \
&& $BAZELISK_CMD build --jobs $JOBS --noenable_bzlmod //cpp:attention_broker_service \
&& mv bazel-bin/cpp/attention_broker_service $BIN_FOLDER \
&& $BAZELISK_CMD build --jobs $JOBS --noenable_bzlmod //cpp:query_broker \
&& mv bazel-bin/cpp/query_broker $BIN_FOLDER \
&& $BAZELISK_CMD build --jobs $JOBS --noenable_bzlmod //cpp:query \
&& mv bazel-bin/cpp/query $BIN_FOLDER

