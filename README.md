# Local SDV Virtualisation Sandbox (vECU)

Local SOME/IP publisher/subscriber sandbox for Windows developers. Run a POSIX vECU environment in Docker/WSL2 without waiting on cloud CI runners.

**Repo:** https://github.com/JackPartridge/SDV_Sandbox (private)

**New here?** Start with [aboutThisProject.md](aboutThisProject.md) for a plain-English explanation. This README is the practical “how to run it” guide.

**Sibling project:** [DigitalTwinDashboard](https://github.com/JackPartridge/DigitalTwinDashboard) — browser UI over the same SOME/IP telemetry (controls, alerts, modern shell).

## Using the SDK (`vecu_sdk`)

Colleagues can publish or subscribe to SOME/IP events **without including vsomeip headers**. Link `libvecu_sdk.so` and include `vecu_sdk.hpp`.

Service IDs and routing are loaded from JSON at startup (no recompile needed when topology changes):

```cpp
#include "vecu_sdk.hpp"

auto config = vecu::loadServiceConfig("/workspace/config/services/canGateway.json");
vecu::VecuPublisher publisher;
publisher.start(*config);
publisher.publish(myBytes);
```

Or keep the defaults (sensor telemetry service `0x1234` / `0x5678`):

```cpp
vecu::VecuSubscriber subscriber;
subscriber.start([](const std::vector<std::uint8_t>& payload) {
  // handle bytes
});
subscriber.requestSetIntervalMs(250);
subscriber.requestResetSequence();
subscriber.requestSetStreaming(false);
```

Override paths with `--service-config <file>` or `VECU_SERVICE_CONFIG`.

See `examples/example_fetcher.cpp` and `config/services/*.json`.

Logging is quiet by default. Pass `--verbose` / `-v` or set `VECU_VERBOSE=1` for diagnostics.

## Prerequisites

- [Docker Desktop](https://www.docker.com/products/docker-desktop/) with WSL2 backend enabled
- Git Bash, PowerShell, or any shell that can run `docker compose`

## Quick start (demo)

```bash
docker compose up --build
```

Publisher offers host/container hardware telemetry (CPU, memory, load, swap, network, process count, temperature when exposed). Subscriber consumes it over SOME/IP.

## CAN → SOME/IP gateway

Simulates legacy CAN frames, decodes them with the Practice Task 1 layout (RPM / speed / steering), and republishes as SOME/IP `vehicleSignal` payloads:

```bash
docker compose run --rm sandbox /workspace/scripts/runCanGateway.sh
```

Headless contract check for the bridge:

```bash
docker compose --profile ci run --rm can-ci-harness
```

## Headless CI/CD harness

No dashboard. Subscribes, validates payloads against a JSON contract, exits `0` (pass) or `1` (fail).

GitLab CI is wired in [`.gitlab-ci.yml`](.gitlab-ci.yml) (build → unit tests → telemetry harness → CAN harness).

Locally:

```bash
./scripts/runDemo.sh ci
```

Or individually:

```bash
docker compose --profile ci run --rm ci-harness
docker compose --profile ci run --rm can-ci-harness
```

## Run the unit tests

```bash
./scripts/runDemo.sh test
```

## Architecture

```text
Windows host
└── Docker container (Ubuntu / POSIX)
    ├── sensorPublisher  --SOME/IP-->  sensorSubscriber / ciContractHarness
    ├── canGateway       --SOME/IP-->  canSubscriber / ciContractHarness
    └── vecu_sdk (JSON service topology)
```

| Component | Role |
|-----------|------|
| `sensorPublisher` | Host/container hardware telemetry over SOME/IP (+ method controls) |
| `canGateway` | CAN simulator → decode → SOME/IP vehicle signals |
| `ciContractHarness` | Headless contract subscriber (exit 0/1) |
| `vecu_sdk` | Shared façade + JSON service config loader |
| `sdvCommon` | Telemetry, CAN decode, vehicle-signal serialisation |

Telemetry wire format is **48 bytes** little-endian (`TelemetryFrame`). Vehicle signals are **28 bytes** (`VehicleSignalFrame`).

## Project layout

```text
config/services/     JSON SOME/IP topology (no recompile)
config/contracts/    CI validation contracts
config/              vsomeip routing JSON
src/publisher/       hardware telemetry vECU
src/subscriber/      classic SOME/IP consumer
src/canGateway/      CAN→SOME/IP microservice
src/ciHarness/       headless integration harness
src/sdk/             vecu_sdk + service config parsing
src/common/          CAN decoder, telemetry, collectors
scripts/             sandbox, CAN demo, CI harness runners
.gitlab-ci.yml       GitLab pipeline (DinD)
```
