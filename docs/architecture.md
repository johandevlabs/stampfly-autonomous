# Architecture

This document ties together the **repository structure**, **control loops**,
**command protocol**, and **safety model** into one cohesive system view.

It is meant to answer:
- What runs where?
- How data flows through the system?
- Where do safety and autonomy live?
- How does a controller interact with the drone?

This file is a **capsule context** document.

---

## One-Sentence Summary

The StampFly firmware is a layered flight stack where **sensors feed estimators**,
**estimators feed controllers**, **controllers feed a mixer**, and **a safety/state
machine supervises everything**, while a remote controller sends **high-level intent**
and receives telemetry.

---

## System Boundaries

### On the drone (authoritative)
- Stability (attitude + rates)
- Altitude & velocity control
- Motion primitives execution
- Safety enforcement and failsafes
- Telemetry generation

### Off the drone (non-authoritative)
- Sends setpoints / intent (z, vx/vy, MOVE_DELTA, LAND)
- Displays and logs telemetry
- Provides a human interface (ESP-NOW controller or Python)

---

## High-Level Block Diagram

```
                ┌───────────────────────────────┐
                │        External Controller     │
                │  (ESP-NOW remote / Python UDP) │
                └───────────────┬───────────────┘
                                │  Commands (intent)
                                ▼
┌─────────────────────────────────────────────────────────────┐
│                          StampFly Drone                       │
│                                                             │
│  ┌───────────┐   ┌───────────────┐   ┌───────────────────┐  │
│  │  Sensors   │→→│  Estimation    │→→│     Control        │  │
│  │ IMU/ToF/   │   │ attitude/z/v  │   │ rate/angle/z/vxy  │  │
│  │ Flow/Baro  │   └───────┬───────┘   └─────────┬─────────┘  │
│  └─────┬──────┘           │                     │            │
│        │                  │                     ▼            │
│        │          ┌───────▼────────┐     ┌───────────────┐   │
│        │          │ Safety + State │────▶│  Mixer + PWM   │──▶│ Motors
│        │          │ Machine        │     └───────────────┘   │
│        │          └───────┬────────┘                          │
│        │                  │                                   │
│        └──────────────────┴───────────► Telemetry ────────────┘
│                                      (state + health + logs)
└─────────────────────────────────────────────────────────────┘
                                ▲
                                │ Telemetry stream
                                └───────────────────────────────
```

---

## Data Flow (Detailed)

### 1) Sensor acquisition
- IMU: gyro + accel (+ optional mag)
- ToF: low altitude range
- Optical flow: ground motion
- Barometer: slow altitude reference
- Battery monitor: voltage/current

Sensors produce **raw measurements** with timestamps.

### 2) Estimation
Estimators convert raw measurements into **state**:

- Attitude estimator → roll/pitch/yaw
- Altitude estimator → z (and optionally z_dot)
- Velocity estimator → vx/vy (from optical flow scaled by z)

Estimators must output:
- values
- health flags
- freshness (age)

### 3) Control (stacked loops)
Controllers generate commands down the stack:

```
Position/Motion (optional)
       ↓
Velocity + Altitude
       ↓
Attitude (angles)
       ↓
Rate (gyro)
       ↓
Mixer
```

### 4) Mixer
Motor outputs computed from:
- throttle
- roll/pitch/yaw torque

Mixer must enforce:
- min/max output
- saturation handling
- disarmed outputs = off

### 5) Safety supervision
Safety is not “a mode”, it is a constant supervisor:
- validates commands
- rejects illegal transitions
- enforces limits
- triggers failsafe/land/kill

---

## Control Stack Diagram (Intent → Motors)

```
  High-level intent (controller)
    z_sp, vx_sp, vy_sp, MOVE_DELTA, LAND
                  │
                  ▼
        ┌──────────────────────┐
        │ Command Interpreter   │   (comms/)
        └──────────┬───────────┘
                   │ setpoints
                   ▼
        ┌──────────────────────┐
        │ State Machine +       │   (state/, safety/)
        │ Safety Guards         │
        └──────────┬───────────┘
                   │ enables
                   ▼
        ┌──────────────────────┐
        │ Position/Motion Layer │   (optional)
        └──────────┬───────────┘
                   │ vx/vy/z setpoints
                   ▼
   ┌─────────────────────────────────────────┐
   │ Outer Loops (20–50 Hz)                  │
   │  - Altitude PID → throttle correction   │
   │  - Velocity PID → roll/pitch setpoints  │
   └───────────────┬─────────────────────────┘
                   │ roll/pitch/yaw setpoints
                   ▼
   ┌─────────────────────────────────────────┐
   │ Attitude Loop (100–200 Hz)              │
   │  - Angle PID → desired rates            │
   └───────────────┬─────────────────────────┘
                   │ rate setpoints
                   ▼
   ┌─────────────────────────────────────────┐
   │ Rate Loop (200–500 Hz)                  │
   │  - Rate PID → torque commands           │
   └───────────────┬─────────────────────────┘
                   │ torques + throttle
                   ▼
        ┌──────────────────────┐
        │ Mixer → Motor outputs │
        └──────────────────────┘
```

---

## Scheduling Model (Why timing matters)

The firmware is time-sliced with strict prioritization:

```
Fast, critical:
  - Rate loop
  - Attitude loop

Medium:
  - Altitude loop
  - Velocity loop
  - State machine

Slow / best-effort:
  - Telemetry
  - Logging
  - Debug printing
```

Rule:
> Telemetry must never block control loops.

---

## Command & Telemetry Interaction

### Command direction (controller → drone)
- Controller sends **intent** packets at ≥10 Hz
- Drone validates and applies safe setpoints
- Drone rejects unsafe mode transitions

### Telemetry direction (drone → controller)
Two streams are recommended:
- Fast (20–50 Hz): attitude, rates, z, vx/vy, mode/state
- Slow (1–5 Hz): battery, sensor health, counters, loop timing

---

## Safety & Failsafe (system-wide)

Safety is enforced at multiple points:

```
Command validation → State guards → Limits → Failsafe overrides → Mixer clamp
```

### Typical failsafe triggers
- Link loss (timeout)
- IMU failure / estimator failure
- Battery low
- Internal timing overruns

Failsafe behavior:
- controlled landing when possible
- KILL always available

---

## Mapping to Repository Structure

This is where the concepts live in code:

| Concept | Folder(s) |
|--------|-----------|
| Sensors | `src/sensors/` |
| Estimation | `src/estimation/` |
| Control loops | `src/control/` |
| State machine | `src/state/` |
| Safety / failsafe | `src/safety/` |
| Command protocol + links | `src/comms/` |
| Telemetry | `src/telemetry/` |
| Scheduling glue | `src/main.cpp` |
| Off-drone tools | `tools/` |

---

## Build & Growth Strategy

The system is designed to grow in phases:

1. Bring-up → sensors + motor tests
2. Attitude stability → rate + angle loops
3. Altitude hold → ToF-based z control
4. Communications → intent + telemetry
5. Velocity hold → optical flow vx/vy
6. Motion primitives → MOVE_DELTA, queues
7. Polish → fusion, robustness

---

## Final Notes

This architecture is successful when it feels:
- **boring**
- **predictable**
- **safe**
- **observable**

If a behavior feels chaotic:
- check estimation
- check timing
- check safety flags
- only then touch PID gains
