#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(git rev-parse --show-toplevel)"

BASE_DIR="/opt"
TMP_DIR="/tmp"

DAS_DIR="${BASE_DIR}/das"
DATA_DIR="${BASE_DIR}/data"
GRPC_DIR="${BASE_DIR}/grpc"
PROTO_DIR="${BASE_DIR}/proto"
BAZEL_DIR="${BASE_DIR}/bazel"
THIRD_PARTY="${BASE_DIR}/3rd-party"

ASSETS_DIR="${REPO_ROOT}/src/assets"

echo "[INFO] Creating base directories under ${BASE_DIR}..."
sudo mkdir -p \
    "${DAS_DIR}" \
    "${DATA_DIR}" \
    "${GRPC_DIR}" \
    "${BAZEL_DIR}" \
    "${PROTO_DIR}" \
    "${THIRD_PARTY}"

echo "[INFO] Updating apt and installing required packages..."
sudo apt update -y
sudo apt install -y \
  git build-essential curl protobuf-compiler python3 python3-pip \
  cmake unzip uuid-runtime lcov bc \
  libevent-dev libssl-dev pkg-config libncurses5

echo "[INFO] Cleaning apt cache..."
sudo apt clean -y || true
sudo rm -rf /var/lib/apt/lists/* || true

echo "[INFO] Installing 3rd-party tools (bazelisk, buildifier)..."

if [[ ! -f "${ASSETS_DIR}/3rd-party.tgz" ]]; then
  echo "[ERROR] ${ASSETS_DIR}/3rd-party.tgz not found."
  exit 1
fi

ARCH="$(uname -m)"
if [[ "${ARCH}" == "aarch64" ]]; then
  ARCH="arm64"
else
  ARCH="amd64"
fi

sudo cp "${ASSETS_DIR}/3rd-party.tgz" "${TMP_DIR}/"
cd "${TMP_DIR}"
tar xzvf 3rd-party.tgz

sudo mv "bazelisk-${ARCH}" "${BAZEL_DIR}/bazelisk"
sudo mv "buildifier-${ARCH}" "${BAZEL_DIR}/buildifier"
sudo chmod +x "${BAZEL_DIR}/"*

echo "[INFO] Installing hiredis-cluster (Redis client)..."

if [[ ! -f "${ASSETS_DIR}/hiredis-cluster.tgz" ]]; then
  echo "[ERROR] ${ASSETS_DIR}/hiredis-cluster.tgz not found."
  exit 1
fi

sudo cp "${ASSETS_DIR}/hiredis-cluster.tgz" "${TMP_DIR}/"
cd "${TMP_DIR}"
tar xzf hiredis-cluster.tgz
cd hiredis-cluster

mkdir -p build
cd build

cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DENABLE_SSL=ON ..
make -j"$(nproc)"
sudo make install

echo "/usr/local/lib" | sudo tee /etc/ld.so.conf.d/local.conf >/dev/null
sudo ldconfig

echo "[INFO] Installing MongoDB C++ driver (mongo-cxx-driver-r4.1.0)..."

if [[ ! -f "${ASSETS_DIR}/mongo-cxx-driver-r4.1.0.tar.gz" ]]; then
  echo "[ERROR] ${ASSETS_DIR}/mongo-cxx-driver-r4.1.0.tar.gz not found."
  exit 1
fi

sudo cp "${ASSETS_DIR}/mongo-cxx-driver-r4.1.0.tar.gz" "${TMP_DIR}/"
cd "${TMP_DIR}"
tar xzvf mongo-cxx-driver-r4.1.0.tar.gz

cd "${TMP_DIR}/mongo-cxx-driver-r4.1.0/build/"
cmake .. -DCMAKE_BUILD_TYPE=Release -DMONGOCXX_OVERRIDE_DEFAULT_INSTALL_PREFIX=OFF
cmake --build . -j"$(nproc)"
sudo cmake --build . --target install

sudo ln -sf /usr/local/include/bsoncxx/v_noabi/bsoncxx/* /usr/local/include/bsoncxx
sudo ln -sf /usr/local/include/bsoncxx/v_noabi/bsoncxx/third_party/mnmlstc/core/ /usr/local/include/core
sudo ln -sf /usr/local/include/mongocxx/v_noabi/mongocxx/* /usr/local/include/mongocxx/
sudo ldconfig

echo "[INFO] Installing cpp-httplib..."

if [[ ! -f "${ASSETS_DIR}/cpp-httplib-master.zip" ]]; then
  echo "[ERROR] ${ASSETS_DIR}/cpp-httplib-master.zip not found."
  exit 1
fi

cp "${ASSETS_DIR}/cpp-httplib-master.zip" "${TMP_DIR}/"
cd "${TMP_DIR}"
unzip -q cpp-httplib-master.zip
sudo cp cpp-httplib-master/httplib.h /usr/local/include/
rm -rf "${TMP_DIR}/cpp-httplib-master"*

echo "[INFO] Creating user 'builder' (if not exists)..."

if ! id "builder" &>/dev/null; then
  sudo useradd -ms /bin/bash builder
fi

echo "[INFO] Configuring git safe.directory for ${DAS_DIR}..."
sudo -u builder git config --global --add safe.directory "${DAS_DIR}"

echo "[INFO] Setup finished."
echo "[INFO] DAS_DIR is: ${DAS_DIR}"
echo "[INFO] You may want to 'cd ${DAS_DIR}' to start working."
