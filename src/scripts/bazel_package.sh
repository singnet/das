#!/bin/bash
set -euo pipefail

BAZELISK_CMD=/opt/bazel/bazelisk
BAZELISK_BUILD_CMD="${BAZELISK_CMD} build --noshow_progress --strategy=CppCompile=standalone --spawn_strategy=standalone"

[ "${BAZEL_JOBS:-x}" != "x" ] && BAZELISK_BUILD_CMD="${BAZELISK_BUILD_CMD} --jobs=${BAZEL_JOBS}"

BAZELISK_RUN_CMD="${BAZELISK_CMD} run"

BUILT_TARGETS_PATH=bazel-bin/

BUILD_BINARIES=false
BUILD_WHEELS=false

EXTERNAL_LIBS_FOLDER="ext_libs"

PACKAGE_TYPE=${1:-none}
shift 1 || true # Clean script args to not mess with the bazel run commands

if [ "$BUILD_BINARIES" = false ] && [ "$BUILD_WHEELS" = false ]; then
    BUILD_BINARIES=true
    BUILD_WHEELS=true
fi

if [ "$BUILD_BINARIES" = true ]; then
    # Binaries
    BUILD_TARGETS+=" //:das"
    BUILD_TARGETS+=" //:attention_broker_service"
    BUILD_TARGETS+=" //:attention_broker_client"
    BUILD_TARGETS+=" //:busnode"
    BUILD_TARGETS+=" //:busclient"

    # Other binaries
    BUILD_TARGETS+=" //:word_query"
    BUILD_TARGETS+=" //:word_query_evolution"
    BUILD_TARGETS+=" //:implication_query_evolution"
    BUILD_TARGETS+=" //:tests_db_loader"
fi

$BAZELISK_BUILD_CMD $BUILD_TARGETS

mkdir -p $EXTERNAL_LIBS_FOLDER

find "$BUILT_TARGETS_PATH" -type f -name "*.so" | while IFS= read -r sofile; do
    ldd "$sofile" | awk '/=> \// { print $3 }' | while IFS= read -r dep; do
        dep_base=$(basename "$dep")
        case "$dep_base" in
            "libmongoc2.so.2"|"libmongocxx.so._noabi"|"libhiredis.so.1.1.0"|"libhiredis_cluster.so.0.12"|"libbson2.so.2"|"libbsoncxx.so._noabi")
                if [ -f "$dep" ]; then
                    cp "$dep" $EXTERNAL_LIBS_FOLDER/
                fi
                ;;
        esac
    done
done

if [[ "$PACKAGE_TYPE" == "deb" ]]; then
    BUILD_TARGETS=" //package:das_deb_package"
    $BAZELISK_BUILD_CMD $BUILD_TARGETS
    cp -L bazel-bin/package/das_1.0.3_amd64.deb $PKG_DIR

elif [[ "$PACKAGE_TYPE" == "rpm" ]]; then
    BUILD_TARGETS=" //package:das_rpm_package"
    $BAZELISK_BUILD_CMD $BUILD_TARGETS
    cp -L bazel-bin/package/das-1.0.3-1.x86_64.rpm $PKG_DIR
fi

rm -rf $EXTERNAL_LIBS_FOLDER
exit $?