# Local SDV Virtualisation Sandbox (vECU)

Local SOME/IP publisher/subscriber sandbox for Windows developers. Run via **Docker** when you have it, or **natively with MinGW/MSYS2** on machines without Docker.

**Repo:** https://github.com/JackPartridge/SDV_Sandbox (private)

**New here?** Start with [aboutThisProject.md](aboutThisProject.md) for a plain-English explanation. This README is the practical “how to run it” guide.

**Sibling project:** [DigitalTwinDashboard](https://github.com/JackPartridge/DigitalTwinDashboard) — browser UI over the same SOME/IP telemetry (controls, alerts, modern shell).

## Two ways to run

| Path | When to use |
|------|-------------|
| **Docker** | Home / laptop with Docker Desktop + WSL2 |
| **Native MinGW** | Work PC without Docker — MSYS2/MINGW64 terminal |

Config JSON uses relative paths (`config/...`) and `127.0.0.1`, so the same files work in both environments. Optional override: `VECU_CONFIG_ROOT` (repo root) and `VSOMEIP_CONFIGURATION`.

---

## Option A — Docker (preferred when available)

### Prerequisites

- [Docker Desktop](https://www.docker.com/products/docker-desktop/) with WSL2 backend
- Git Bash, PowerShell, or any shell that can run `docker compose`

### Quick start

```bash
docker compose up --build
```

### CAN gateway / CI / unit tests

```bash
docker compose run --rm sandbox /workspace/scripts/runCanGateway.sh
./scripts/runDemo.sh ci
./scripts/runDemo.sh test
```

---

## Option B — Native Windows (MinGW / MSYS2, no Docker)

Use a **MINGW64** shell from [MSYS2](https://www.msys2.org/).

### 1. Install Windows build tools

```bash
pacman -S --needed mingw-w64-x86_64-cmake mingw-w64-x86_64-boost \
  mingw-w64-x86_64-gcc mingw-w64-x86_64-make mingw-w64-x86_64-pkgconf git
```

Verify:

```bash
cmake --version
g++ --version
```

### 2. Build vsomeip from source (~10 minutes)

COVESA vsomeip supports Windows but is not shipped as a ready MinGW package. From the repo root:

```bash
./scripts/buildVsomeipMingw.sh
```

This clones vsomeip **3.7.5** into `deps/vsomeip-src` and installs into `deps/vsomeip-prefix`. Confirm a DLL exists, for example:

```bash
find deps/vsomeip-prefix -name '*vsomeip3*.dll'
```

### 3. Configure and build this sandbox

```bash
./scripts/buildNativeMingw.sh
```

Or manually:

```bash
export VECU_VSOMEIP_PREFIX="$PWD/deps/vsomeip-prefix"
cmake -S . -B build-native -G "MinGW Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$VECU_VSOMEIP_PREFIX"
cmake --build build-native --parallel
```

CMake links `vsomeip3` and, on Windows, `ws2_32` (Winsock) for every SOME/IP target.

### 4. Run on the Windows loopback

From the **repo root** (so relative `config/` paths resolve):

```bash
export VECU_CONFIG_ROOT="$PWD"
export VSOMEIP_CONFIGURATION="$PWD/config/vsomeipLocal.json"
export PATH="$PWD/deps/vsomeip-prefix/bin:/mingw64/bin:$PATH"

./scripts/runNativeSandbox.sh
```

Or run a single binary:

```bash
./build-native/exampleFetcher.exe
./build-native/canGateway.exe --service-config config/services/canGateway.json
./build-native/ciContractHarness.exe \
  --service-config config/services/sensorSubscriber.json \
  --contract config/contracts/telemetryContract.json
```

`unicast` in the vsomeip JSON files is already `127.0.0.1` — do not point them at a Docker bridge IP.

**Note:** `/proc` / thermal metrics are Linux-oriented. On native Windows the publisher still speaks SOME/IP; CPU/memory gauges may read as zero/unavailable unless you run under Docker/WSL. The CAN gateway and contract harnesses work fully without Docker.

---

## Using the SDK (`vecu_sdk`)

Colleagues can publish or subscribe to SOME/IP events **without including vsomeip headers**. Link `vecu_sdk` and include `vecu_sdk.hpp`.

Service IDs and routing are loaded from JSON at startup (no recompile when topology changes):

```cpp
#include "vecu_sdk.hpp"

auto config = vecu::loadServiceConfig("config/services/canGateway.json");
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

Override paths with `--service-config <file>`, `VECU_SERVICE_CONFIG`, or `VECU_CONFIG_ROOT`.

See `examples/example_fetcher.cpp` and `config/services/*.json`.

Logging is quiet by default. Pass `--verbose` / `-v` or set `VECU_VERBOSE=1` for diagnostics.

## Headless CI/CD harness

No dashboard. Subscribes, validates payloads against a JSON contract, exits `0` (pass) or `1` (fail).

GitLab CI is wired in [`.gitlab-ci.yml`](.gitlab-ci.yml) (Docker-in-Docker).

Docker locally:

```bash
./scripts/runDemo.sh ci
```

## Architecture

```text
Windows host
├── Docker (optional): Ubuntu container + vsomeip
└── Native MinGW: vsomeip3.dll + sensorPublisher / canGateway / ciContractHarness
       └── SOME/IP on 127.0.0.1
```

| Component | Role |
|-----------|------|
| `sensorPublisher` | Host/container hardware telemetry over SOME/IP (+ method controls) |
| `canGateway` | CAN simulator → decode → SOME/IP vehicle signals |
| `ciContractHarness` | Headless contract subscriber (exit 0/1) |
| `vecu_sdk` | Shared façade + JSON service config loader |
| `sdvCommon` | Telemetry, CAN decode, vehicle-signal serialisation, path helpers |

Telemetry wire format is **48 bytes** little-endian (`TelemetryFrame`). Vehicle signals are **28 bytes** (`VehicleSignalFrame`).

## Project layout

```text
config/services/     JSON SOME/IP topology (no recompile)
config/contracts/    CI validation contracts
config/              vsomeip routing JSON (127.0.0.1)
src/publisher/       hardware telemetry vECU
src/subscriber/      classic SOME/IP consumer
src/canGateway/      CAN→SOME/IP microservice
src/ciHarness/       headless integration harness
src/sdk/             vecu_sdk + service config parsing
src/common/          CAN decoder, telemetry, collectors, platform paths
scripts/             Docker demos, CI, MinGW vsomeip + native build
deps/                local vsomeip build/install (gitignored)
.gitlab-ci.yml       GitLab pipeline (DinD)
```
