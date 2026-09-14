#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

export VSOMEIP_CONFIGURATION="${ROOT_DIR}/config/vsomeipCanBridge.json"

cleanup() {
  if [[ -n "${GATEWAY_PID:-}" ]]; then
    kill "${GATEWAY_PID}" 2>/dev/null || true
    sleep 0.2
    kill -9 "${GATEWAY_PID}" 2>/dev/null || true
    wait "${GATEWAY_PID}" 2>/dev/null || true
  fi
}
trap cleanup EXIT INT TERM

echo "CAN CI: starting canGateway..."
VSOMEIP_APPLICATION_NAME=canGateway \
  "${BUILD_DIR}/canGateway" \
  --service-config "${ROOT_DIR}/config/services/canGateway.json" &
GATEWAY_PID=$!
sleep 1

echo "CAN CI: validating vehicle-signal SOME/IP contract..."
set +e
VSOMEIP_APPLICATION_NAME=canSubscriber \
  "${BUILD_DIR}/ciContractHarness" \
  --service-config "${ROOT_DIR}/config/services/canSubscriber.json" \
  --contract "${ROOT_DIR}/config/contracts/vehicleSignalContract.json"
STATUS=$?
set -e

cleanup
trap - EXIT INT TERM
exit "${STATUS}"
