#!/bin/bash

if [ $1 == "--no-destroy" ]; then
  docker stop docker-das-alpine-1
else
  # Faster test reruns
  docker compose -f tests/integration/docker/compose_das_alpine.yaml down
fi

