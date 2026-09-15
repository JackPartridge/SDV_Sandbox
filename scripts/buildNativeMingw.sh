#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

PREFIX="${VECU_VSOMEIP_PREFIX:-${ROOT_DIR}/deps/vsomeip-prefix}"
export VECU_VSOMEIP_PREFIX="${PREFIX}"
export VECU_CONFIG_ROOT="${ROOT_DIR}"
export PATH="/mingw64/bin:${PATH}"

if [[ ! -d "${PREFIX}" ]]; then
  echo "vsomeip prefix missing — building it first..."
  "${ROOT_DIR}/scripts/buildVsomeipMingw.sh"
fi

echo "Configuring SDV sandbox (MinGW)..."
cmake -S . -B build-native \
  -G "MinGW Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="${PREFIX}"

cmake --build build-native --parallel "$(nproc 2>/dev/null || echo 4)"

echo
echo "Native build ready under build-native/"
echo "Example (from repo root):"
echo "  export VSOMEIP_CONFIGURATION=${ROOT_DIR}/config/vsomeipLocal.json"
echo "  export PATH=\"${PREFIX}/bin:\${PATH}\""
echo "  ./build-native/sensorPublisher.exe"
