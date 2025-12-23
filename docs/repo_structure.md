# Repository Structure & Design Rationale

This document describes the **recommended repository structure** for the
StampFly semi-autonomous drone project, along with the reasoning behind it.

It serves both as:
- A contributor guide
- A **capsule context** document that can be shared with LLMs or collaborators

---

## Top-Level Structure

```
stampfly-autonomous/
│
├── README.md
├── platformio.ini
├── .gitignore
│
├── docs/
├── src/
├── test/
└── tools/
```

---

## Design Philosophy

This structure follows a few core principles:

1. **Separation of concerns**
2. **Control logic is independent of sensors and comms**
3. **`main.cpp` stays boring**
4. **Safety is explicit, not implicit**
5. **Easy to grow without refactoring**

The layout is inspired by PX4 / ArduPilot, but intentionally simplified for
an ESP32 + Arduino environment.

---

## `docs/` — Project Knowledge & Context

```
docs/
├── architecture.md
├── control_loops.md
├── command_protocol.md
├── safety_model.md
├── pid_tuning.md
├── telemetry.md
└── repo_structure.md   ← this file
```

Purpose:
- Capture *intent*, not just code
- Avoid knowledge living only in someone’s head
- Provide reusable context for future prompts and discussions

Highly recommended to keep these files short and updated.

---

## `src/` — Flight Firmware

### Overview

```
src/
├── main.cpp
├── config/
├── board/
├── sensors/
├── estimation/
├── control/
├── state/
├── comms/
├── telemetry/
├── safety/
└── utils/
```

Each folder answers one clear question:

| Folder | Responsibility |
|------|----------------|
| `config/` | Pins, constants, defaults |
| `board/` | Board-level init & power |
| `sensors/` | Raw sensor drivers |
| `estimation/` | State estimation |
| `control/` | PID + control laws |
| `state/` | Flight modes & state machine |
| `comms/` | Command links |
| `telemetry/` | Reporting & logging |
| `safety/` | Failsafes & limits |
| `utils/` | Shared helpers |

---

## `main.cpp` — The Boring Glue

`main.cpp` should:
- Initialize subsystems
- Schedule control loops
- Route data between modules

It should **not**:
- Contain control math
- Talk directly to sensors
- Implement safety logic

If `main.cpp` becomes clever, something belongs elsewhere.

---

## Sensors vs Estimation vs Control

This separation is critical:

```
Sensors → Estimation → Control → Actuators
```

- **Sensors**: raw measurements (IMU, ToF, flow)
- **Estimation**: “what do we believe the state is?”
- **Control**: “what should we do about it?”

80% of flight bugs originate in estimation, not PID tuning.

---

## `comms/` — Transport Is Not Policy

```
comms/
├── espnow_link.cpp
├── udp_link.cpp
├── packet_defs.h
└── comms.h
```

Rules:
- Packet formats are shared
- Transport is swappable
- Control logic never cares *how* commands arrived

This allows:
- ESP-NOW controllers
- Python UDP scripts
- Future BLE or serial links

---

## `state/` — Explicit Flight Modes

Flight behavior is governed by a clear state machine:

Examples:
- IDLE
- ARMING
- TAKEOFF
- ALT_HOLD
- VEL_HOLD
- LANDING
- FAILSAFE

Avoid “mode flags scattered everywhere”. Centralize behavior.

---

## `safety/` — Non-Negotiable

Safety code lives in its own module to ensure:
- Visibility
- Auditability
- No accidental bypass

Includes:
- Link-loss handling
- Tilt/throttle limits
- Kill switch
- Battery failsafes

If safety logic is implicit, it will be violated eventually.

---

## `tools/` — Everything Off-Drone

```
tools/
├── python/
│   ├── controller.py
│   ├── telemetry_plot.py
│   └── packet_sniffer.py
│
└── esp32_controller/
    └── platformio.ini
```

This keeps:
- Firmware clean
- Experiments fast
- Debugging ergonomic

---

## Roadmap Mapping

| Phase | Main folders touched |
|-----|----------------------|
| Bring-up | `board/`, `sensors/` |
| Attitude | `estimation/`, `control/` |
| Altitude | `control/altitude*`, `state/` |
| Comms | `comms/`, `telemetry/` |
| XY Motion | `sensors/flow`, `control/velocity*` |
| Autonomy | `state/`, `control/`, `comms/` |

---

## Final Notes

This structure is intentionally:
- Boring
- Explicit
- Verbose

That’s what makes it scalable, debuggable, and teachable.

If the project succeeds, this repo structure can serve as the foundation for
other micro-flight or robotics projects.
