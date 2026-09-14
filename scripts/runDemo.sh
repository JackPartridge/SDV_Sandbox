#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

MODE="${1:-demo}"

case "${MODE}" in
  demo|up)
    docker compose up --build sandbox
    ;;
  test|tests)
    docker compose build sandbox
    docker compose --profile test run --rm unit-tests
    ;;
  ci)
    docker compose build sandbox
    docker compose --profile ci run --rm --no-TTY ci-harness
    docker compose --profile ci run --rm --no-TTY can-ci-harness
    ;;
  build)
    docker compose build sandbox
    ;;
  down|stop)
    docker compose down --remove-orphans
    ;;
  *)
    echo "Usage: $0 [demo|test|ci|build|down]"
    echo "  demo   Build (if needed) and launch publisher + subscriber (default)"
    echo "  test   Build and run the behavioural unit-test suite"
    echo "  ci     Build and run headless telemetry + CAN contract harnesses"
    echo "  build  Build the sandbox image only"
    echo "  down   Stop and remove compose resources"
    exit 1
    ;;
esac
