#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CONFIG_PATH="${VSOMEIP_CONFIGURATION:-${ROOT_DIR}/config/vsomeipLocal.json}"
BUILD_DIR="${ROOT_DIR}/build"

export VSOMEIP_CONFIGURATION="${CONFIG_PATH}"

if [[ ! -x "${BUILD_DIR}/sensorPublisher" || ! -x "${BUILD_DIR}/sensorSubscriber" ]]; then
  echo "Binaries not found under ${BUILD_DIR}."
  echo "Build first: cmake -S . -B build && cmake --build build"
  exit 1
fi

cleanup() {
  if [[ -n "${PUBLISHER_PID:-}" ]] && kill -0 "${PUBLISHER_PID}" 2>/dev/null; then
    kill "${PUBLISHER_PID}" 2>/dev/null || true
    wait "${PUBLISHER_PID}" 2>/dev/null || true
  fi
  if [[ -n "${SUBSCRIBER_PID:-}" ]] && kill -0 "${SUBSCRIBER_PID}" 2>/dev/null; then
    kill "${SUBSCRIBER_PID}" 2>/dev/null || true
    wait "${SUBSCRIBER_PID}" 2>/dev/null || true
  fi
}
trap cleanup EXIT INT TERM

echo "Starting SDV sandbox with config: ${VSOMEIP_CONFIGURATION}"

VSOMEIP_APPLICATION_NAME=sensorPublisher "${BUILD_DIR}/sensorPublisher" &
PUBLISHER_PID=$!

sleep 1

VSOMEIP_APPLICATION_NAME=sensorSubscriber "${BUILD_DIR}/sensorSubscriber" &
SUBSCRIBER_PID=$!

echo "Sandbox running (publisher=${PUBLISHER_PID}, subscriber=${SUBSCRIBER_PID}). Ctrl+C to stop."
wait
