#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VSOMEIP_SRC="${VECU_VSOMEIP_SRC:-${ROOT_DIR}/deps/vsomeip-src}"
PREFIX="${VECU_VSOMEIP_PREFIX:-${ROOT_DIR}/deps/vsomeip-prefix}"
VSOMEIP_VERSION="${VSOMEIP_VERSION:-3.7.5}"

mkdir -p "${ROOT_DIR}/deps"

if [[ ! -d "${VSOMEIP_SRC}/.git" ]]; then
  echo "Cloning vsomeip ${VSOMEIP_VERSION}..."
  git clone --depth 1 --branch "${VSOMEIP_VERSION}" \
    https://github.com/COVESA/vsomeip.git "${VSOMEIP_SRC}"
fi

cmake -S "${VSOMEIP_SRC}" -B "${VSOMEIP_SRC}/build" \
  -G "MinGW Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
  -DENABLE_SIGNAL_HANDLING=1

cmake --build "${VSOMEIP_SRC}/build" --parallel "$(nproc 2>/dev/null || echo 4)"
cmake --install "${VSOMEIP_SRC}/build"

echo "vsomeip installed to: ${PREFIX}"
echo "Look for vsomeip3.dll under ${PREFIX}/bin (or lib)."
find "${PREFIX}" -name 'vsomeip3.dll' -o -name 'libvsomeip3.dll' 2>/dev/null | head -5 || true
