#!/usr/bin/env bash

set -euo pipefail

detect_host_arch() {
  case "$(uname -m)" in
    x86_64)
      echo "amd64"
      ;;
    aarch64 | arm64)
      echo "arm64"
      ;;
    *)
      uname -m
      ;;
  esac
}

REPO_ROOT="$(git rev-parse --show-toplevel)"
BAZEL_WORKSPACE="$REPO_ROOT/src"

HOST_ARCH="$(detect_host_arch)"
ARCH="$HOST_ARCH"

TMP_ARGS=()
for arg in "$@"; do
  case "$arg" in
    --arch=*)
      ARCH="${arg#--arch=}"
      ;;
    *)
      TMP_ARGS+=("$arg")
      ;;
  esac
done
set -- "${TMP_ARGS[@]}"

BAZELISK_CMD=${BAZELISK_CMD:-/opt/bazel/bazelisk}

cd "$BAZEL_WORKSPACE"

if [ ! -x "$BAZELISK_CMD" ]; then
  echo "[ERROR] bazelisk not found or not executable at: $BAZELISK_CMD"
  exit 1
fi

BIN_DIR=${BIN_DIR:-"$REPO_ROOT/bin/$ARCH"}
LIB_DIR=${LIB_DIR:-"$REPO_ROOT/lib/$ARCH"}

echo "[INFO] Using ARCH=$ARCH"
echo "[INFO] Using BIN_DIR=$BIN_DIR"
echo "[INFO] Using LIB_DIR=$LIB_DIR"

mkdir -p "$BIN_DIR" "$LIB_DIR"

BAZELISK_BUILD_CMD="${BAZELISK_CMD} build --noshow_progress --strategy=CppCompile=standalone --spawn_strategy=standalone"
[ "${BAZEL_JOBS:-x}" != "x" ] && BAZELISK_BUILD_CMD="${BAZELISK_BUILD_CMD} --jobs=${BAZEL_JOBS}"
BAZELISK_RUN_CMD="${BAZELISK_CMD} run"

BAZEL_BINARY_TARGETS=(
  "//:inference_agent_server"
  "//:inference_agent_client"
  "//:link_creation_server"
  "//:link_creation_agent_client"
  "//:word_query"
  "//:word_query_evolution"
  "//:implication_query_evolution"
  "//:attention_broker_service"
  "//:attention_broker_client"
  "//:query_broker"
  "//:evolution_broker"
  "//:evolution_client"
  "//:query"
  "//:das"
  "//:tests_db_loader"
  "//:context_broker"
  "//:atomdb_broker"
  "//:atomdb_broker_client"
)

BAZEL_BINARY_OUTPUTS=(
  "bazel-bin/inference_agent_server"
  "bazel-bin/inference_agent_client"
  "bazel-bin/link_creation_server"
  "bazel-bin/link_creation_agent_client"
  "bazel-bin/word_query"
  "bazel-bin/word_query_evolution"
  "bazel-bin/implication_query_evolution"
  "bazel-bin/attention_broker_service"
  "bazel-bin/attention_broker_client"
  "bazel-bin/query_broker"
  "bazel-bin/evolution_broker"
  "bazel-bin/evolution_client"
  "bazel-bin/query"
  "bazel-bin/tests_db_loader"
  "bazel-bin/context_broker"
  "bazel-bin/atomdb_broker"
  "bazel-bin/atomdb_broker_client"
)

BAZEL_LIB_OUTPUTS=(
  "bazel-bin/hyperon_das.so"
)

REQUIRED_SO_DEPS=(
  "libmongoc2.so.2"
  "libmongocxx.so._noabi"
  "libhiredis.so.1.1.0"
  "libhiredis_cluster.so.0.12"
  "libbson2.so.2"
  "libbsoncxx.so._noabi"
)

BUILD_BINARIES=false
BUILD_WHEELS=false

ARGS=()
for arg in "$@"; do
  case "$arg" in
    --binaries)
      BUILD_BINARIES=true
      ;;
    --wheels)
      BUILD_WHEELS=true
      ;;
    *)
      ARGS+=("$arg")
      ;;
  esac
done

set -- "${ARGS[@]}"

if [ "$BUILD_BINARIES" = false ] && [ "$BUILD_WHEELS" = false ]; then
  BUILD_BINARIES=true
  BUILD_WHEELS=true
fi

BUILD_TARGETS=()

if [ "$BUILD_BINARIES" = true ]; then
  BUILD_TARGETS+=("${BAZEL_BINARY_TARGETS[@]}")
fi

if [ "$BUILD_WHEELS" = true ]; then
  echo "[INFO] Running bazelisk to update Python client requirements..."
  $BAZELISK_RUN_CMD //deps:requirements_python_client.update
fi

if [ "${#BUILD_TARGETS[@]}" -eq 0 ]; then
  echo "[INFO] No Bazel targets to build. Exiting."
  exit 0
fi

echo "[INFO] Building Bazel targets:"
for t in "${BUILD_TARGETS[@]}"; do
  echo "  - $t"
done

$BAZELISK_BUILD_CMD "${BUILD_TARGETS[@]}" "$@"

if [ "$BUILD_BINARIES" = true ]; then
  echo "[INFO] Moving binaries to $BIN_DIR..."
  for bin in "${BAZEL_BINARY_OUTPUTS[@]}"; do
    if [ -f "$bin" ]; then
      mv "$bin" "$BIN_DIR"
      echo "  -> moved $(basename "$bin")"
    else
      echo "[WARN] Binary not found, skipping: $bin"
    fi
  done

  echo "[INFO] Moving libraries to $LIB_DIR..."
  for lib in "${BAZEL_LIB_OUTPUTS[@]}"; do
    if [ -f "$lib" ]; then
      mv "$lib" "$LIB_DIR"
      echo "  -> moved $(basename "$lib")"
    else
      echo "[WARN] Library not found, skipping: $lib"
    fi
  done
fi

echo "[INFO] Copying required shared library dependencies into $LIB_DIR..."

if [ -d "$LIB_DIR" ]; then
  find "$LIB_DIR" -type f -name "*.so" | while IFS= read -r sofile; do
    ldd "$sofile" | awk '/=> \// { print $3 }' | while IFS= read -r dep; do
      dep_base=$(basename "$dep")
      for wanted in "${REQUIRED_SO_DEPS[@]}"; do
        if [ "$dep_base" = "$wanted" ]; then
          if [ -f "$dep" ]; then
            cp -u "$dep" "$LIB_DIR"
            echo "  -> copied $dep_base"
          fi
        fi
      done
    done
  done
fi

echo "[INFO] Build finished successfully."
echo "[INFO] Binaries in: $BIN_DIR"
echo "[INFO] Libraries in: $LIB_DIR"

exit 0
