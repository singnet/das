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
EXTERNAL_LIBS_PATH="package/$EXTERNAL_LIBS_FOLDER"

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
    BUILD_TARGETS+=" //:tests_db_loader"
    BUILD_TARGETS+=" //:database_adapter"
fi

$BAZELISK_BUILD_CMD $BUILD_TARGETS

mkdir -p $EXTERNAL_LIBS_PATH

find "$BUILT_TARGETS_PATH" -type f -name "*.so" | while IFS= read -r sofile; do
    ldd "$sofile" | awk '/=> \// { print $3 }' | while IFS= read -r dep; do
        dep_base=$(basename "$dep")
        case "$dep_base" in
            "libmongoc2.so.2"|"libmongocxx.so._noabi"|"libhiredis.so.1.1.0"|"libhiredis_cluster.so.0.12"|"libbson2.so.2"|"libbsoncxx.so._noabi")
                if [ -f "$dep" ]; then
                    cp "$dep" $EXTERNAL_LIBS_PATH/
                fi
                ;;
        esac
    done
done

find "$BUILT_TARGETS_PATH" -type f -executable -name "database_adapter" | while IFS= read -r binfile; do
    ldd "$binfile" | awk '/=> \// { print $3 }' | while IFS= read -r dep; do
        dep_base=$(basename "$dep")
        case "$dep_base" in
            "libpq.so.5")
                if [ -f "$dep" ]; then
                    cp -f "$dep" "$EXTERNAL_LIBS_PATH"
                fi
                ;;
        esac
    done
done

if [[ "$PACKAGE_TYPE" == "deb" ]]; then
    BUILD_TARGETS=" //package:das_deb_package"
    $BAZELISK_BUILD_CMD $BUILD_TARGETS

    LATEST_DEB=$(ls -t bazel-bin/package/*.deb | head -n 1)
    cp -L "$LATEST_DEB" "$PKG_DIR/das_package.deb"

elif [[ "$PACKAGE_TYPE" == "rpm" ]]; then
    BUILD_TARGETS=" //package:das_rpm_package"
    $BAZELISK_BUILD_CMD $BUILD_TARGETS

    LATEST_RPM=$(ls -t bazel-bin/package/*.rpm | head -n 1)
    cp -L "$LATEST_RPM" "$PKG_DIR/das_package.rpm" 
fi

rm -rf "$EXTERNAL_LIBS_PATH"
exit $?
