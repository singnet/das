#!/bin/bash -x

(( JOBS=$(nproc)/2 ))
BAZELISK_CMD=/opt/bazel/bazelisk
BAZELISK_TEST_CMD="${BAZELISK_CMD} test --jobs ${JOBS} --enable_bzlmod --test_output=errors --build_tests_only"

$BAZELISK_TEST_CMD //...

