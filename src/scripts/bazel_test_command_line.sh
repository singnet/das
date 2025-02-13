#!/bin/bash

echo '============================================================================================================================'; 
if [ -z "$2" ]
    then
        echo "Running all tests in $1"
        /opt/bazel/bazelisk test --jobs 6 --noenable_bzlmod tests:$1
    else
        echo "Running $2 in $1"
        /opt/bazel/bazelisk test --jobs 6 --noenable_bzlmod --test_arg=--gtest_filter=$2 tests:$1
fi
