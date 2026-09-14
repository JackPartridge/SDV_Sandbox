#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

export VSOMEIP_CONFIGURATION="${VSOMEIP_CONFIGURATION:-${ROOT_DIR}/config/vsomeipLocal.json}"

cleanup() {
  if [[ -n "${PUBLISHER_PID:-}" ]]; then
    kill "${PUBLISHER_PID}" 2>/dev/null || true
    sleep 0.2
    kill -9 "${PUBLISHER_PID}" 2>/dev/null || true
    wait "${PUBLISHER_PID}" 2>/dev/null || true
  fi
}
trap cleanup EXIT INT TERM

echo "CI harness: starting sensorPublisher..."
VSOMEIP_APPLICATION_NAME=sensorPublisher "${BUILD_DIR}/sensorPublisher" &
PUBLISHER_PID=$!
sleep 1

echo "CI harness: running headless contract check..."
set +e
"${BUILD_DIR}/ciContractHarness" \
  --service-config "${ROOT_DIR}/config/services/sensorSubscriber.json" \
  --contract "${ROOT_DIR}/config/contracts/telemetryContract.json"
STATUS=$?
set -e

cleanup
trap - EXIT INT TERM

if [[ "${STATUS}" -eq 0 ]]; then
  echo "CI harness: PASS"
else
  echo "CI harness: FAIL (exit ${STATUS})"
fi
exit "${STATUS}"
