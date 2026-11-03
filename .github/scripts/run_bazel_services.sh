#!/usr/bin/env bash
# Load a MeTTa file with the Bazel db_loader and start the Bazel attention broker.
# src/bazel-bin/db_loader and src/bazel-bin/attention_broker_service must already be built.
#
# Connection settings come from DAS_REDIS_* and DAS_MONGODB_*. The rest of the
# loader config is config/das.json.
#
# Usage: run_bazel_services.sh <metta-file> [attention-port]
set -euo pipefail

if [[ $# -lt 1 || $# -gt 2 ]]; then
  echo "Usage: $0 <metta-file> [attention-port]" >&2
  exit 1
fi

METTA_FILE=$1
ATTENTION_PORT=${2:-40001}

for var in DAS_REDIS_PORT DAS_MONGODB_PORT DAS_MONGODB_USERNAME DAS_MONGODB_PASSWORD; do
  if [[ -z "${!var:-}" ]]; then
    echo "[ERROR] ${var} is required." >&2
    exit 1
  fi
done

REPO_ROOT="$(git rev-parse --show-toplevel)"
DB_LOADER="${REPO_ROOT}/src/bazel-bin/db_loader"
ATTENTION_BROKER="${REPO_ROOT}/src/bazel-bin/attention_broker_service"
LOADER_CONFIG=${REPO_ROOT}/config/das.json
ATTENTION_LOG="/tmp/attention_broker_service.log"
ATTENTION_PID="/tmp/attention_broker_service.pid"

for bin in "${DB_LOADER}" "${ATTENTION_BROKER}"; do
  if [[ ! -x "${bin}" ]]; then
    echo "[ERROR] Missing Bazel binary: ${bin}" >&2
    exit 1
  fi
done

if [[ ! -f "${METTA_FILE}" ]]; then
  echo "[ERROR] MeTTa file not found: ${METTA_FILE}" >&2
  exit 1
fi

# /proc/<pid>/stat field 22 (starttime). Paired with the PID, this stays valid
# when the broker executable is replaced and rejects a reused PID.
broker_start_ticks() {
  local pid=$1
  [[ "${pid}" =~ ^[0-9]+$ ]] || return 1
  [[ -r "/proc/${pid}/stat" ]] || return 1
  awk '
    {
      rest = $0
      sub(/^.*\) /, "", rest)
      n = split(rest, f, " ")
      if (n >= 20 && f[20] ~ /^[0-9]+$/) {
        print f[20]
        exit 0
      }
      exit 1
    }
  ' "/proc/${pid}/stat"
}

echo "[INFO] Loading ${METTA_FILE} with ${DB_LOADER}"
"${DB_LOADER}" --config="${LOADER_CONFIG}" --file="${METTA_FILE}"

if command -v das-cli >/dev/null 2>&1; then
  das-cli attention-broker stop >/dev/null 2>&1 || true
fi
if [[ -f "${ATTENTION_PID}" ]]; then
  old_pid=""
  old_start=""
  read -r old_pid old_start _ < "${ATTENTION_PID}" || true
  if [[ "${old_pid}" =~ ^[0-9]+$ && "${old_start}" =~ ^[0-9]+$ ]]; then
    current_start="$(broker_start_ticks "${old_pid}" || true)"
    if [[ -n "${current_start}" && "${current_start}" == "${old_start}" ]]; then
      kill -TERM "${old_pid}" >/dev/null 2>&1 || true
    fi
  fi
  rm -f "${ATTENTION_PID}"
fi
for _ in $(seq 1 30); do
  if ! (echo >/dev/tcp/127.0.0.1/"${ATTENTION_PORT}") >/dev/null 2>&1; then
    break
  fi
  if [[ "${_}" -eq 30 ]]; then
    echo "[ERROR] Port ${ATTENTION_PORT} is already in use by another process." >&2
    exit 1
  fi
  sleep 1
done

echo "[INFO] Starting ${ATTENTION_BROKER} on 0.0.0.0:${ATTENTION_PORT}"
: > "${ATTENTION_LOG}"
setsid "${ATTENTION_BROKER}" "0.0.0.0:${ATTENTION_PORT}" >>"${ATTENTION_LOG}" 2>&1 </dev/null &
broker_pid=$!
broker_start="$(broker_start_ticks "${broker_pid}" || true)"
if [[ -z "${broker_start}" ]]; then
  echo "[ERROR] Attention broker exited before listening." >&2
  cat "${ATTENTION_LOG}" >&2 || true
  exit 1
fi
printf '%s %s\n' "${broker_pid}" "${broker_start}" > "${ATTENTION_PID}"

for _ in $(seq 1 30); do
  if (echo >/dev/tcp/127.0.0.1/"${ATTENTION_PORT}") >/dev/null 2>&1; then
    echo "[INFO] Attention broker is listening on port ${ATTENTION_PORT}"
    exit 0
  fi
  if [[ "$(broker_start_ticks "${broker_pid}" || true)" != "${broker_start}" ]]; then
    echo "[ERROR] Attention broker exited before listening." >&2
    cat "${ATTENTION_LOG}" >&2 || true
    exit 1
  fi
  sleep 1
done

echo "[ERROR] Attention broker did not listen on port ${ATTENTION_PORT}." >&2
cat "${ATTENTION_LOG}" >&2 || true
exit 1
