#!/bin/bash

set -e

BAZEL_CMD="/opt/bazel/bazelisk"
export USE_BAZEL_VERSION="${USE_BAZEL_VERSION:-9.0.0}"
export BAZELISK_BASE_URL="${BAZELISK_BASE_URL:-https://github.com/bazelbuild/bazel/releases/download}"

$BAZEL_CMD $([ ${BAZEL_JOBS:-x} != x ] && echo --jobs=${BAZEL_JOBS}) "$@"


if [ "$LIB_DIR" != "" ]; then
    find bazel-bin/ -type f -name "*.so" | while IFS= read -r sofile; do
        cp -f "$sofile" "$LIB_DIR"
        ldd "$sofile" | awk '/=> \// { print $3 }' | while IFS= read -r dep; do
            if [ -f "$dep" ]; then
                cp -v "$dep" "$LIB_DIR"
            fi
        done
    done
fi

if [ "$BIN_DIR" != "" ]; then
    find bazel-bin/ -type f ! -name "*.*" | while IFS= read -r file; do
        cp -f "$file" "$BIN_DIR"
    done
fi

echo $?
