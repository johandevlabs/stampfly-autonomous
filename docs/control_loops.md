# Control Loops Overview

This document defines the **control loop architecture**, execution rates,
and responsibilities for the StampFly semi-autonomous drone.

It is intended as:
- A design reference
- A tuning guide
- A capsule context file for future prompts or contributors

The structure is inspired by classical multirotor flight stacks
(PX4 / ArduPilot), simplified for ESP32-class hardware.

---

## Control Stack Overview

The flight controller is organized as **stacked control loops**:

```
          User / Autonomy Commands
                     ↓
            Position / Motion Logic
                     ↓
        Velocity & Altitude Controllers
                     ↓
          Attitude (Angle) Controller
                     ↓
           Rate (Gyro) Controller
                     ↓
              Motor Mixer
                     ↓
                 Motors
```

Each layer:
- Runs at a different frequency
- Has a single, well-defined responsibility
- Exposes clear inputs and outputs

---

## Loop Timing Summary

| Loop | Frequency | Purpose |
|----|-----------|---------|
| Rate control (gyro) | 200–500 Hz | Stabilize angular rates |
| Attitude control | 100–200 Hz | Stabilize roll/pitch/yaw |
| Altitude control | 20–50 Hz | Maintain target height |
| Velocity control | 20–50 Hz | Control horizontal motion |
| Position / motion | 5–20 Hz | Execute motion primitives |
| State machine | 10–50 Hz | Mode transitions & safety |
| Telemetry | 10–50 Hz (fast), 1–5 Hz (slow) | Reporting |

Exact rates may be adjusted based on CPU load.

---

## Rate Control Loop (Inner Loop)

**Frequency:** 200–500 Hz  
**Criticality:** Highest

### Inputs
- Gyroscope angular rates (p, q, r)
- Desired angular rates from attitude controller

### Outputs
- Torque commands → motor mixer

### Controller
- PID per axis (rate PID)

### Notes
- This loop must always run on time
- Keep logic minimal
- No dynamic memory, no logging

If this loop is unstable, nothing above it matters.

---

## Attitude (Angle) Control Loop

**Frequency:** 100–200 Hz

### Inputs
- Estimated attitude (roll, pitch, yaw)
- Desired attitude (or desired rates)

### Outputs
- Desired angular rates

### Controller
- PID per axis (angle PID)

### Notes
- Converts “where we want to point” into rate setpoints
- Yaw may be simplified or rate-only initially

---

## Altitude Control Loop (Z-axis)

**Frequency:** 20–50 Hz

### Inputs
- Estimated altitude (z)
- Target altitude (z_setpoint)

### Outputs
- Throttle correction (Δthrottle)

### Sensors
- Primary: ToF (VL53L3)
- Secondary (optional): Barometer (BMP280)

### Controller
- PID on altitude or vertical velocity

### Notes
- Ground effect dominates below ~10 cm
- Start testing at 15–20 cm
- Clamp throttle aggressively

---

## Horizontal Velocity Control Loop (X/Y)

**Frequency:** 20–50 Hz

### Inputs
- Estimated horizontal velocity (vx, vy)
- Velocity setpoints (vx_sp, vy_sp)

### Outputs
- Desired roll and pitch angles

### Sensors
- Optical flow (PMW3901)
- Scaled using altitude estimate

### Controller
- PID per axis (velocity PID)

### Notes
- Keep angles small (<10–15°)
- Expect drift; absolute position is not guaranteed

---

## Position / Motion Layer

**Frequency:** 5–20 Hz

### Responsibilities
- Interpret high-level commands:
  - MOVE_DELTA(dx, dy, dz)
  - HOLD_POSITION
  - LAND
- Generate velocity and altitude setpoints
- Enforce motion limits

### Notes
- This layer is *not* a PID
- It produces smooth, time-based setpoints
- Think “motion planner lite”

---

## State Machine

**Frequency:** 10–50 Hz

### Example States
- IDLE
- ARMING
- TAKEOFF
- ALT_HOLD
- VEL_HOLD
- LANDING
- FAILSAFE

### Responsibilities
- Enforce valid transitions
- Activate/deactivate controllers
- Handle link loss and faults

State logic should never live inside control loops.

---

## Telemetry Loop

### Fast Telemetry (10–50 Hz)
- Attitude
- Rates
- Altitude
- Velocity
- Throttle
- Active mode

### Slow Telemetry (1–5 Hz)
- Battery voltage/current
- Sensor health
- CPU load
- Loop timing

Telemetry must never block control loops.

---

## Scheduling Model (ESP32)

Recommended approach:
- Single main loop
- Time-sliced execution using `micros()` or RTOS tasks
- Hard priority for rate + attitude loops

Example (conceptual):

```
loop():
  run_rate_loop_if_due()
  run_attitude_loop_if_due()
  run_altitude_loop_if_due()
  run_velocity_loop_if_due()
  run_state_machine_if_due()
  run_telemetry_if_due()
```

---

## Design Rules (Do Not Break)

1. Inner loops must never depend on outer loops
2. Control loops never allocate memory
3. Logging and comms are always lower priority
4. Safety overrides everything
5. Estimation errors masquerade as PID problems

---

## Common Failure Modes

| Symptom | Likely Cause |
|------|--------------|
| Oscillation | P too high / estimator noise |
| Slow response | P too low / D too high |
| Drift | Estimation bias |
| Sudden jumps | Sensor glitches |
| Random crashes | Timing overruns |

---

## Final Notes

A boring, predictable control loop is a **successful** control loop.

If something feels chaotic:
- Slow down
- Log more
- Check estimation before touching PID gains
