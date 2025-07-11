#!/bin/bash

set -uo pipefail

RANGE_ARG="${1:-}"

if [[ "$RANGE_ARG" =~ ^--range=([0-9]+)$ ]]; then
  PORT_RANGE_SIZE="${BASH_REMATCH[1]}"
  JOIN_CMD="gkctl instance join --range=${PORT_RANGE_SIZE}"
  RESERVE_CMD="gkctl port reserve --range=${PORT_RANGE_SIZE}"
else
  JOIN_CMD="gkctl instance join"
  RESERVE_CMD="gkctl port reserve"
fi

gkctl instance list --current > /dev/null 2>&1
if [ $? -ne 0 ]; then
  echo "[INFO] Current instance not found. Joining..."
  eval "$JOIN_CMD"
else
  echo "[INFO] Current instance already exists."
fi

echo "[INFO] Reserving port..."
eval "$RESERVE_CMD"
