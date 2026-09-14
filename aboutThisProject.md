# About this project

This document is the plain-English guide to **what this sandbox is**, **why it exists**, and **why that is useful**.  
As the project grows, update this file first so newcomers always get the “so what?” before the technical detail.

For how to build and run things, see [README.md](README.md).

---

## In one sentence

This project lets a developer on a Windows laptop run a small, realistic **virtual car computer setup** locally — two software “ECUs” talking to each other the way real vehicle services do — instead of waiting on cloud build machines every time they want to check if something works.

---

## What it is

Think of a modern car as a network of small computers (ECUs). Software in those computers does not only “call a function”; it often **finds a service**, **subscribes to data**, and **receives updates** over an automotive middleware protocol called **SOME/IP**.

This repository is a **local sandbox** that recreates a tiny slice of that world:

| Piece | Plain meaning |
|--------|----------------|
| **Publisher (sensor node)** | Pretends to be a sensor ECU. It samples host/container metrics and offers them as a service. |
| **CAN→SOME/IP gateway** | Ingests simulated legacy CAN frames, decodes RPM/speed/steering, republishes as SOME/IP. |
| **Subscriber / CI harness** | Discovers services and consumes events — interactively or headlessly with exit code 0/1. |
| **JSON service config** | Topology (service/instance/event IDs) loaded at startup so routes change without recompiling. |
| **SOME/IP (via vsomeip)** | The “vehicle network language” they use to find each other and exchange messages. |
| **Docker + Linux** | The box those programs run in, so we get a real POSIX/Linux environment on a Windows PC. |
| **Contract / unit tests** | Checks that shared data formats are exactly what we claim (byte layout, values, edge cases). |
| **Compose / demo scripts** | One-command way for a developer to build, test, and watch the sides talk. |

It is **not** a full vehicle, a full digital twin, or a replacement for every CI check.  
It **is** a focused, local rehearsal of “two vECUs discovering each other and exchanging sensor data correctly.”

---

## What it does (the story of a run)

1. You start the sandbox (usually with Docker Compose).
2. The **publisher** starts up, registers with the SOME/IP runtime, and **offers** a sensor service.
3. The **subscriber** starts up, **asks** for that service, **subscribes** to its events, and waits.
4. Once discovery succeeds, the publisher repeatedly sends **hardware telemetry** (sequence, CPU %, memory %, temperature when available, timestamp).
5. The subscriber receives each notification, checks the 24-byte payload, and prints the decoded values. Method requests can change publish interval, reset sequence, or pause streaming.
6. Separately, the **test suite** proves the payload format is stable and correct *before* you even trust the network path.

If those steps work on your machine, you already know a lot: the middleware stack is behaving, the data contract holds, and the two processes can cooperate inside a Linux environment — without a car and without a cloud queue.

---

## Why we built it

Today, a lot of software checking for this kind of work still depends on **shared cloud runners** (for example GitLab CI):

- You change code.
- You push.
- You wait in a queue.
- A remote machine builds and tests.
- Eventually you learn whether a simple mistake was obvious.

That feedback loop is slow and expensive for early mistakes.  
This sandbox supports a **shift-left** approach: catch the “does our local SOME/IP path even work?” questions **on the developer’s PC**, early and often.

So the “why” is practical:

- **Faster learning** when something is wrong.
- **Less waiting** on shared infrastructure for basic middleware checks.
- **Lower cloud cost** for work that does not need a full pipeline yet.
- **Shared understanding** of what “good” looks like for a tiny vECU conversation.

---

## Why this is useful (and not “just Docker”)

Anyone can run Ubuntu in Docker. That only gives you a Linux box.

What makes this useful is what runs **inside** that box:

1. **Two roles, not one process** — a producer and a consumer, like separate ECUs.
2. **Dynamic discovery** — the subscriber must find and subscribe to the service; data is not merely “broadcast into the void” like a simplistic demo.
3. **A real automotive protocol stack** — SOME/IP via vsomeip, not a homemade TCP toy that only looks similar.
4. **A hard data contract** — a fixed binary layout that tests prove, so both sides agree on meaning, not just “some bytes arrived.”
5. **Repeatable developer packaging** — build, test, and demo with the same Compose/Docker flow.

In short: Docker is the **delivery van**. The sandbox is the **mini vehicle network experiment** in the back of the van.

---

## What “good” looks like

When this project is healthy, you should be able to say:

- **Contract is solid** — tests pass against expected byte values and rules, not against “whatever the code happens to do today.”
- **Discovery works** — logs show offer → request/find → available → subscribe → acknowledge.
- **Data matches** — what the publisher emits for a given sequence is what the subscriber decodes.
- **A new developer can start quickly** — one documented command path, without needing deep CI knowledge first.

---

## What we have today

Keep this section short and current. Update it whenever the sandbox gains a meaningful capability.

- Local Ubuntu-based Docker image with the C++ toolchain and vsomeip.
- C++20 publisher and subscriber applications.
- **`vecu_sdk` shared library** — publish and subscribe without vsomeip headers (Pimpl).
- Shared domain logic for sensor simulation and payload serialisation.
- Behavioural unit tests run during image build and via Compose.
- `docker compose` demo plus helper scripts for day-to-day use.
- Quiet-by-default logging with optional `--verbose` / `VECU_VERBOSE`.
- Example hardware-fetcher app (`exampleFetcher`) showing clean SDK publisher usage.

Related sibling project: `../DigitalTwinDashboard` — browser gauges over WebSocket fed by `VecuSubscriber`.

---

## What this is not (yet)

Being honest helps people trust the doc:

- Not a full SDV platform or complete ECU software stack.
- Not a substitute for formal vehicle network design (ARXML, production configs, security, etc.).
- Not a claim that local success means production readiness — it means **early confidence**, not final sign-off.
- Not multi-host vehicle network simulation beyond this contained sandbox (unless/until we add that).

---

## How to talk about it to others

Useful short versions depending on audience:

- **To a developer:** “Run two SOME/IP microservices locally in Docker and prove discovery plus payload correctness before you push.”
- **To a lead / manager:** “Shift-left testing for SDV middleware so the team spends less time waiting on cloud runners for basic service communication checks.”
- **To a non-specialist:** “A laptop-sized practice version of two car computers finding each other and sharing sensor data, so engineers can check the basics without booking a lab or a cloud machine.”

---

## Living document notes

When you add something meaningful to the sandbox, update:

1. **What we have today** — new capability in plain English.
2. **What this is not (yet)** — remove or reword anything that becomes true.
3. **What “good” looks like** — only if the definition of success changes.

Technical commands, file maps, and IDs belong in [README.md](README.md). This file stays about meaning and value.
