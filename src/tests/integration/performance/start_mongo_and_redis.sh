#!/bin/bash
# This script is used to start MongoDB and Redis for performance testing.

# start MongoDB
sudo rm -rf /tmp/mongodb-data /tmp/mongodb-configdb \
  && sudo mkdir -p /tmp/mongodb-data /tmp/mongodb-configdb \
  && sudo tar jvxf /opt/das/data/perf-test/mongodb-data.tar.bz2 -C /tmp/mongodb-data/ \
  && sudo chown -R 999:999 /tmp/mongodb-data /tmp/mongodb-configdb

docker stop mongodb-perf-tests-38000 || true && \
docker rm mongodb-perf-tests-38000 || true && \
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

# start Redis
sudo rm -rf /tmp/redis-data \
  && sudo mkdir -p /tmp/redis-data \
  && sudo tar jvxf /opt/das/data/perf-test/redis-data.tar.bz2 -C /tmp/redis-data/ \
  && sudo chown -R 999:1000 /tmp/redis-data
  
docker stop redis-perf-tests-39000 || true && \
docker rm redis-perf-tests-39000 || true && \
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

