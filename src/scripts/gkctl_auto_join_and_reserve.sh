#!/bin/bash

set -euo pipefail

gkctl instance list --current > /dev/null 2>&1
if [ $? -ne 0 ]; then
  echo "[INFO] Current instance not found. Joining..."
  gkctl instance join
else
  echo "[INFO] Current instance already exists."
fi

echo "[INFO] Reserving port..."
gkctl port reserve
