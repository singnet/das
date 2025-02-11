#!/bin/bash
MAX_TRIES=50 # 5 minutes
TRY_COUNT=0
LOG=""
on_failure() {
  echo "$LOG"
  exit 1
}
PORT=$(
  comm -23 \
    <(seq 1024 65535 | sort) \
    <(ss -tuln | awk '{print $5}' | cut -d: -f2 | sort) \
    | shuf -n 1)
export FAAS_PORT=${PORT}
if [ $1 == "--build" ]; then
  LOG=$(
    docker compose -f \
      tests/integration/docker/compose_das_alpine.yaml \
      up -d --force-recreate --build 2>&1)
else
  LOG=$(
    docker compose -f \
      tests/integration/docker/compose_das_alpine.yaml \
      up -d --force-recreate 2>&1)
fi

if [ $? -ne 0 ]; then
  on_failure
fi

until [ $TRY_COUNT -ge $MAX_TRIES ] \
  || [ "$(docker inspect --format '{{.State.Health.Status}}' docker-das-alpine-1)" == "healthy" ]
do
  TRY_COUNT=$((TRY_COUNT + 1))
  sleep 6
done

if [ $TRY_COUNT -ge $MAX_TRIES ]; then
  echo "ERROR: Timeout"
  exit 1
fi

echo "${PORT}"
