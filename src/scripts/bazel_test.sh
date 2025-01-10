#!/bin/bash -x

(( JOBS=$(nproc)/2 ))
BAZELISK_CMD=/opt/bazel/bazelisk
REPO_ROOT=/opt/das-attention-broker
WORKSPACE_DIR=${REPO_ROOT}/cpp

cd $WORKSPACE_DIR
$BAZELISK_CMD test \
    --jobs $JOBS \
    --noenable_bzlmod \
    --cache_test_results=no \
    //...

