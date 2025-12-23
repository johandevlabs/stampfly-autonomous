# StampFly Semi-Autonomous Drone Project

## Project Vision
Build a **semi-autonomous micro-drone** based on the M5Stamp Fly platform where:
- The **drone handles low-level intelligence** (stabilization, altitude, position, safety).
- The **user provides high-level intent** (target altitude, move delta-x/y, land).
- The system is robust to link loss and prioritizes safety.

This project is intentionally educational, inspired by PX4/ArduPilot concepts but scaled down to ESP32 + Arduino.

---

> This repository follows a documented architecture.
> Start with `docs/architecture.md`.

---

## Documentation

- [Repository structure & design rationale](docs/repo_structure.md)
- [Repository architecture](docs/architecture.md)
- [Control loops](docs/control_loops.md)
- [Safety model](docs/safety_model.md)


---

## System Architecture

### On-Drone Responsibilities
- Sensor drivers
- State estimation
- Control loops (PID-based)
- Safety logic and state machine
- Command interpreter
- Telemetry streaming

### Off-Drone Responsibilities
- Command sender (ESP-NOW or UDP)
- Telemetry receiver / logger
- Optional visualization
- No safety-critical logic

---

## Phased Roadmap

### Phase 0 — Bring-up & Hardware Confidence
**Goal:** Full confidence in hardware and peripherals.

**Features:**
- Flash baseline firmware (M5 or minimal custom)
- Enumerate all sensors
- Verify sensor update rates
- Individual motor spin test
- LED and buzzer tests

**Exit Criteria:**
- All sensors read reliably
- Motors respond correctly
- No brown-outs or unexpected resets

---

### Phase 1 — Attitude Stabilization (Core Flight)
**Goal:** Drone stabilizes itself in roll, pitch, and yaw.

**Features:**
- IMU driver (gyro + accelerometer)
- Attitude estimation (complementary / Mahony / Madgwick)
- Rate PID (p, q, r)
- Angle PID (roll, pitch, yaw)
- Motor mixer
- ARM / DISARM logic

**Safety:**
- Max tilt angle limits
- Throttle limits
- Immediate KILL command

**Exit Criteria:**
- Stable behavior on test stand
- Correct reaction to disturbances
- Hover possible with manual throttle

---

### Phase 2 — Altitude Hold
**Goal:** Stable Z-axis control without user throttle.

**Features:**
- ToF distance sensor driver
- Altitude estimation
- Altitude PID → throttle correction
- Takeoff / hover / land state machine
- Ground-effect damping

**Telemetry:**
- z, z_setpoint, throttle, PID terms

**Exit Criteria:**
- Holds 15–20 cm for ≥10 seconds
- Repeatable takeoff and landing
- Optional test at ~5 cm altitude

---

### Phase 3 — Communications & Telemetry
**Goal:** Remote high-level control and visibility.

**Features:**
- Command protocol (versioned)
- ESP-NOW and/or UDP support
- Heartbeat + timeout detection
- Structured telemetry streaming

**Commands:**
- ARM / DISARM
- SET_MODE
- SET_Z
- LAND
- KILL

**Exit Criteria:**
- Drone responds reliably to remote commands
- Link loss triggers controlled LAND
- Telemetry logs are consistent and readable

---

### Phase 4 — Horizontal Velocity Control
**Goal:** Controlled motion in X/Y plane.

**Features:**
- Optical flow driver
- Velocity estimation (vx, vy)
- Velocity PID → roll/pitch setpoints
- Velocity limits and damping
- Mode: VEL_HOLD

**Exit Criteria:**
- Drone roughly holds position indoors
- Smooth response to vx/vy commands
- No runaway drift

---

### Phase 5 — Motion Primitives
**Goal:** Abstract low-level control into intent-based commands.

**Features:**
- MOVE_DELTA(dx, dy, dz)
- Time-based velocity profiles
- Simple command queue
- Chained commands

**Exit Criteria:**
- Repeatable “move forward X cm” behavior
- Autonomous execution without manual intervention

---

### Phase 6 — Robustness & Polish
**Goal:** Make the system solid and demo-ready.

**Features:**
- Improved sensor fusion
- Adaptive PID tuning
- Logging to flash
- Flight replay
- LED status language
- Low-battery behavior
- Runtime configuration

**Exit Criteria:**
- Safe, predictable behavior
- Trustworthy enough to demo

---

## Feature Dependency Map

```
Sensors → Estimation → Control → Safety → Comms → Autonomy
```

If behavior is unstable:
- 80% estimation issue
- 15% PID tuning
- 5% software bug

---

## Suggested Documentation (Highly Recommended)
- `ControlLoops.md` — loop rates and responsibilities
- `CommandProtocol.md`
- `SafetyModel.md`
- Telemetry logs (`.csv`)
- PID tuning notes

---

## License
MIT (suggested)

---

## Status
🚧 Early development — expect frequent changes.
