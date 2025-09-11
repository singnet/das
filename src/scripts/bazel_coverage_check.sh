#!/usr/bin/env bash
set -euo pipefail
set -x

SCRIPT_DIR="$(dirname "$0")"
REPORT="bazel-out/_coverage/_coverage_report.dat"
THRESHOLD=70

bash "$SCRIPT_DIR/bazel_exec.sh" coverage --combined_report=lcov --keep_going --show_progress //tests/...

if [[ -z "$REPORT" ]]; then
  echo "[Error] coverage file not found!"
  exit 1
fi


COVERAGE=$(lcov --summary "$REPORT" | grep "lines......" | awk '{print $2}' | sed 's/%//')

echo "Line coverage: $COVERAGE%"

if (( $(echo "$COVERAGE < $THRESHOLD" | bc -l) )); then
    echo "[Error]: line coverage $COVERAGE% is below threshold of $THRESHOLD%"
    exit 1
fi

echo "Coverage OK"
