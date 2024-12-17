#!/bin/bash

rm -f ../bin/* && ../scripts/bazel_build.sh && ../bin/attention_broker 37007
