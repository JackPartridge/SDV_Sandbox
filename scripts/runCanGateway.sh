#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

export VSOMEIP_CONFIGURATION="${ROOT_DIR}/config/vsomeipCanBridge.json"

echo "Starting CAN→SOME/IP gateway (Ctrl+C to stop)..."
VSOMEIP_APPLICATION_NAME=canGateway \
  "${BUILD_DIR}/canGateway" \
  --service-config "${ROOT_DIR}/config/services/canGateway.json" \
  "$@"
