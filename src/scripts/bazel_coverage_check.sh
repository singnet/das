#!/usr/bin/env bash
set -euo pipefail
set -x

SCRIPT_DIR="$(dirname "$0")"
REPORT_PATH="bazel-out/_coverage/_coverage_report.dat"
MARKDOWN_REPORT="coverage-report.md"

if [ -f "$REPORT_PATH" ]; then
  echo "Removing $REPORT_PATH"
  rm "$REPORT_PATH"
fi

bash "$SCRIPT_DIR/bazel_exec.sh" coverage --keep_going --show_progress //tests/...

if [[ -f "$REPORT_PATH" ]]; then
  echo "Coverage report generated at: $REPORT_PATH"

  gcovr --coveralls "$REPORT_PATH" --markdown "$MARKDOWN_REPORT"

  echo "Markdown report generated at: $MARKDOWN_REPORT"
else
  echo "Coverage report not found"
  exit 1
fi
