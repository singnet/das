#!/bin/bash -x

BAZEL_CMD="/opt/bazel/bazelisk"

$BAZEL_CMD $([ ${BAZEL_JOBS:-x} != x ] && echo --jobs=${BAZEL_JOBS}) "$@"


find bazel-bin/ -type f -name "*.so" | while IFS= read -r sofile; do
    cp -f "$sofile" "$LIB_DIR"
    ldd "$sofile" | awk '/=> \// { print $3 }' | while IFS= read -r dep; do
        if [ -f "$dep" ]; then
            cp -v "$dep" "$LIB_DIR"
        fi
    done
done


find bazel-bin/ -type f ! -name "*.*" | while IFS= read -r file; do
    cp -f "$file" "$BIN_DIR"
done

echo $?
