#!/usr/bin/env bash
set -euo pipefail

# Extra arguments are forwarded to bazelisk before the build verb, for example:
#   BAZELISK_HOME=... .github/scripts/start_attention_broker.sh --output_user_root=...

REPO_ROOT="$(git rev-parse --show-toplevel)"
cd "${REPO_ROOT}/src"

PORT=40001
ADDRESS="0.0.0.0:${PORT}"
PID_FILE=/tmp/attention-broker.pid
LOG_FILE=/tmp/attention-broker.log

das-cli attention-broker stop || true
if [[ -f "${PID_FILE}" ]]; then
  kill "$(cat "${PID_FILE}")" >/dev/null 2>&1 || true
  rm -f "${PID_FILE}"
fi

./scripts/bazel_exec.sh "$@" build --noshow_progress //:attention_broker_service

setsid nohup ./bazel-bin/attention_broker_service "${ADDRESS}" >"${LOG_FILE}" 2>&1 </dev/null &
echo $! > "${PID_FILE}"

for _ in $(seq 1 60); do
  if (echo >/dev/tcp/127.0.0.1/"${PORT}") >/dev/null 2>&1; then
    echo "Attention broker ready on port ${PORT}"
    exit 0
  fi
  if ! kill -0 "$(cat "${PID_FILE}")" >/dev/null 2>&1; then
    echo "Attention broker exited" >&2
    cat "${LOG_FILE}" >&2 || true
    exit 1
  fi
  echo "Waiting for attention broker..."
  sleep 1
done

echo "Attention broker failed to start" >&2
cat "${LOG_FILE}" >&2 || true
exit 1
