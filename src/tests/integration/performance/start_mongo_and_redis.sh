#!/bin/bash

set -eou pipefail

DATA_FOLDER="/home/ubuntu/.cache/das"

# This script is used to start MongoDB and Redis for performance testing.

function stop_container() {
  CONTAINER_NAME="$1"
  echo "===== Removing existing container ${CONTAINER_NAME} if it exists ====="
  if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "Stopping and removing existing container: ${CONTAINER_NAME}"
    _=$(docker stop "${CONTAINER_NAME}" 2>&1 > /dev/null)
    _=$(docker rm "${CONTAINER_NAME}" 2>&1 > /dev/null)
  else
    echo "No existing container found with name: ${CONTAINER_NAME}"
  fi
}

# Set up MongoDB ===================================================================================

MONGO_CONTAINER_NAME="mongodb-perf-tests-38000"
stop_container "${MONGO_CONTAINER_NAME}"
echo

MONGO_TMP_DATA="/tmp/mongodb-data"
MONGO_TMP_CONFIGDB="/tmp/mongodb-configdb"
echo "===== Setting up MongoDB data on ${MONGO_TMP_DATA} ====="
sudo rm -rf "${MONGO_TMP_DATA}" "${MONGO_TMP_CONFIGDB}"
sudo mkdir -p "${MONGO_TMP_DATA}" "${MONGO_TMP_CONFIGDB}"
sudo pv "${DATA_FOLDER}/mongodb-data.tar.bz2" | tar jxf - -C "${MONGO_TMP_DATA}/"
sudo chown -R 999:999 "${MONGO_TMP_DATA}" "${MONGO_TMP_CONFIGDB}"
echo

echo "===== Starting MongoDB on port 38000 ====="
docker run \
  --name "mongodb-perf-tests-38000" \
  --runtime "runc" \
  --log-driver "json-file" \
  --restart "on-failure:5" \
  --publish "0.0.0.0:38000:27017/tcp" \
  --network "bridge" \
  --hostname "mongodb-perf-tests" \
  --expose "27017/tcp" \
  --env "MONGO_INITDB_ROOT_USERNAME=dbadmin" \
  --env "MONGO_INITDB_ROOT_PASSWORD=dassecret" \
  --env "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin" \
  --env "GOSU_VERSION=1.17" \
  --env "JSYAML_VERSION=3.13.1" \
  --env "MONGO_PACKAGE=mongodb-org" \
  --env "MONGO_REPO=repo.mongodb.org" \
  --env "MONGO_MAJOR=6.0" \
  --env "MONGO_VERSION=6.0.13" \
  --env "HOME=/data/db" \
  --volume "/tmp/mongodb-data:/data/db" \
  --volume "/tmp/mongodb-configdb:/data/configdb" \
  --label "org.opencontainers.image.ref.name"="ubuntu" \
  --label "org.opencontainers.image.version"="22.04" \
  --detach \
  --entrypoint "docker-entrypoint.sh" \
  "mongo:6.0.13-jammy" \
  "mongod"

echo "MongoDB started on port 38000 (container: ${MONGO_CONTAINER_NAME})"

echo
echo

# Set up Redis =====================================================================================

REDIS_CONTAINER_NAME="redis-perf-tests-39000"
stop_container "${REDIS_CONTAINER_NAME}"
echo

REDIS_TMP_DATA="/tmp/redis-data"
echo "===== Setting up Redis data on ${REDIS_TMP_DATA} ====="
sudo rm -rf "${REDIS_TMP_DATA}"
sudo mkdir -p "${REDIS_TMP_DATA}"
sudo pv "${DATA_FOLDER}/redis-data.tar.bz2" | tar jxf - -C "${REDIS_TMP_DATA}/"
sudo chown -R 999:1000 "${REDIS_TMP_DATA}"
echo

echo "===== Starting Redis on port 39000 ====="  
docker run \
  --name "redis-perf-tests-39000" \
  --runtime "runc" \
  --log-driver "json-file" \
  --restart "on-failure:5" \
  --network "host" \
  --hostname "redis-perf-tests" \
  --expose "39000/tcp" \
  --env "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin" \
  --env "GOSU_VERSION=1.17" \
  --env "REDIS_VERSION=7.2.3" \
  --env "REDIS_DOWNLOAD_URL=http://download.redis.io/releases/redis-7.2.3.tar.gz" \
  --env "REDIS_DOWNLOAD_SHA=3e2b196d6eb4ddb9e743088bfc2915ccbb42d40f5a8a3edd8cb69c716ec34be7" \
  --volume "/tmp/redis-data:/data" \
  --detach \
  --entrypoint "docker-entrypoint.sh" \
  "redis:7.2.3-alpine" \
  "redis-server" "--port" "39000" "--appendonly" "yes" "--protected-mode" "no"

echo "Redis started on port 39000 (container: ${REDIS_CONTAINER_NAME})"