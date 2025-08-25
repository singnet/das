#!/usr/bin/env bash
set -euo pipefail
set -x

TEST_TARGETS=(
  "//..."
)

BAZEL_CMD="/opt/bazel/bazelisk"

JOBS_ARG=()
if [[ -n "${BAZEL_JOBS:-}" ]]; then
  JOBS_ARG=(--jobs="$BAZEL_JOBS")
fi

set +e
$BAZEL_CMD "${JOBS_ARG[@]}" coverage \
  "${TEST_TARGETS[@]}" \
  --combined_report=lcov \
  --keep_going \
  --test_output=errors
BAZEL_EXIT_CODE=$?
set -e

if [[ $BAZEL_EXIT_CODE -ne 0 ]]; then
  echo "Some tests failed (exit code $BAZEL_EXIT_CODE), continuing to generate coverage report..."
fi

REPORT_PATH="bazel-out/_coverage/_coverage_report.dat"
HTML_DIR="coverage-report"

if [[ -f "$REPORT_PATH" ]]; then
  echo "Coverage report generated at: $REPORT_PATH"

  genhtml "$REPORT_PATH" --output-directory "$HTML_DIR"

  echo "HTML report generated at: $HTML_DIR/index.html"
else
  echo "Coverage report not found"
  exit 1
fi
