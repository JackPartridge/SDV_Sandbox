#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build-native"
PREFIX="${VECU_VSOMEIP_PREFIX:-${ROOT_DIR}/deps/vsomeip-prefix}"

export VECU_CONFIG_ROOT="${ROOT_DIR}"
export VSOMEIP_CONFIGURATION="${VSOMEIP_CONFIGURATION:-${ROOT_DIR}/config/vsomeipLocal.json}"
export PATH="${PREFIX}/bin:/mingw64/bin:${PATH}"

if [[ ! -x "${BUILD_DIR}/sensorPublisher.exe" && ! -x "${BUILD_DIR}/sensorPublisher" ]]; then
  echo "Native binaries not found. Run scripts/buildNativeMingw.sh first."
  exit 1
fi

PUB="${BUILD_DIR}/sensorPublisher.exe"
SUB="${BUILD_DIR}/sensorSubscriber.exe"
[[ -x "${PUB}" ]] || PUB="${BUILD_DIR}/sensorPublisher"
[[ -x "${SUB}" ]] || SUB="${BUILD_DIR}/sensorSubscriber"

cleanup() {
  if [[ -n "${PUBLISHER_PID:-}" ]]; then
    kill "${PUBLISHER_PID}" 2>/dev/null || true
    sleep 0.2
    kill -9 "${PUBLISHER_PID}" 2>/dev/null || true
    wait "${PUBLISHER_PID}" 2>/dev/null || true
  fi
}
trap cleanup EXIT INT TERM

echo "Starting native publisher/subscriber on 127.0.0.1..."
VSOMEIP_APPLICATION_NAME=sensorPublisher "${PUB}" &
PUBLISHER_PID=$!
sleep 1
VSOMEIP_APPLICATION_NAME=sensorSubscriber "${SUB}"
