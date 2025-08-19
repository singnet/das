#!/bin/bash -x
set -euo pipefail

TEST_TARGETS=(
  "//..."
)

bash ./src/scripts/bazel.sh coverage "${TEST_TARGETS[@]}" --combined_report=lcov --keep_going
