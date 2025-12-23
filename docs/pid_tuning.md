# PID Tuning Guide

This document describes a practical, repeatable approach to tuning the control
loops for the StampFly semi-autonomous drone.

It is intended to be used alongside:
- `control_loops.md`
- `safety_model.md`
- flight telemetry logs

The goal is not “perfect control” — it is **stable, predictable behavior**.

---

## Safety First (Before Any Tuning)

Before tuning with props on:
- Verify sensor readings are sane (no NaNs, no jumps)
- Verify motor order and direction
- Verify ARM/DISARM/KILL behaviors
- Set conservative hard limits:
  - max tilt angle
  - max throttle
  - max altitude
- Prefer a tether / test stand for first flights

Never tune without:
- a KILL command
- a link-loss LAND behavior
- a way to log key variables

---

## Recommended Tuning Order

Tune from the inside out:

1. **Rate PID (gyro loop)**
2. **Angle PID (attitude loop)**
3. **Altitude PID**
4. **Velocity PID (optical flow)**

Outer loops are meaningless if inner loops are unstable.

---

## What to Log

At minimum, log these for each tuned loop:

### Rate loop
- p/q/r measured
- p/q/r setpoints
- PID terms (P, I, D)
- motor outputs
- loop dt (microseconds)

### Angle loop
- roll/pitch/yaw estimated
- roll/pitch/yaw setpoints
- generated rate setpoints

### Altitude loop
- z measured
- z setpoint
- throttle baseline + correction
- ToF quality/status (if available)

### Velocity loop
- vx, vy measured
- vx, vy setpoints
- roll/pitch output setpoints

If you can’t see it, you can’t tune it.

---

## General PID Behaviors (Mental Model)

| Gain | Increases | Too much looks like |
|------|-----------|---------------------|
| P | responsiveness | oscillation, twitching |
| I | removes steady bias | slow wobble, wind-up |
| D | damping | noise amplification, jitter |

Many “PID problems” are actually estimation problems.

---

## Loop 1: Rate PID Tuning (Inner Loop)

**Goal:** Commanded rates are followed quickly with minimal overshoot.

### Setup
- Disable angle controller (or feed direct rate setpoints)
- Start with I = 0, D = 0

### Procedure
1. Increase **P** slowly until you see fast response and *slight* oscillation.
2. Add **D** to reduce oscillation and overshoot.
3. Add small **I** only if there’s consistent bias (rare on rate loop).

### Signs
- **Too much P:** high-frequency oscillation
- **Too much D:** noisy motor outputs, “buzzing”
- **Too much I:** slow growing oscillation after a few seconds

### Anti-windup
Implement:
- integrator clamp
- disable I accumulation when output saturated

---

## Loop 2: Angle PID Tuning (Attitude Loop)

**Goal:** Drone points where requested without wobble.

### Setup
- Rate loop must already be stable.
- Start with P only, I=0, D=0.

### Procedure
1. Increase **angle P** until attitude follows commands crisply.
2. Add **angle D** if needed (often optional in angle loop).
3. Add a small **angle I** only if it doesn’t hold level.

### Notes
Yaw can be:
- rate-only initially (simplest)
- later upgraded to heading hold with magnetometer

---

## Loop 3: Altitude PID Tuning

**Goal:** Hold a target height without pumping or drifting.

### Strong recommendation
Start at **15–20 cm**, not 5 cm, due to ground effect.

### Setup
- Altitude loop outputs throttle correction around a `hover_throttle` baseline
- Use ToF as primary below ~1 m

### Procedure
1. Start with P only.
2. Increase P until it responds but begins to bounce.
3. Add D to reduce bounce/overshoot.
4. Add I to eliminate steady offset (if needed).

### Special: ground effect
Below ~10 cm:
- lift increases unexpectedly
- response becomes non-linear

Mitigations:
- reduced authority near ground
- stronger damping
- avoid initiating horizontal motion below ~10 cm

---

## Loop 4: Velocity PID Tuning (Optical Flow)

**Goal:** Hold vx/vy near zero (no drift) and track commanded velocities.

### Setup
- Requires stable attitude + altitude hold
- Set a conservative max tilt (e.g., 10–15°)

### Procedure
1. Tune for **hold still** (vx_sp = vy_sp = 0).
2. Increase P until drift is reduced but oscillation begins.
3. Add D sparingly (optical flow can be noisy).
4. Add I only if persistent drift remains.

### Notes
Optical flow velocity scaling depends on altitude.
If altitude estimate is wrong, velocity loop will behave strangely.

---

## Step Response Tests (Recommended)

For each loop, do controlled tests:
- small step in setpoint
- observe overshoot, settling time, oscillation

Examples:
- roll setpoint: 0 → 5° → 0
- z setpoint: 20 cm → 25 cm → 20 cm
- vx setpoint: 0 → 10 cm/s → 0

Record logs and compare changes.

---

## Practical Defaults & Constraints

Even without final numbers, enforce constraints:
- limit integral term magnitude
- clamp total output
- low-pass filter D input (derivative on measurement)
- set minimum dt and maximum dt checks

---

## Common Tuning Mistakes

1. Tuning angle loop before rate loop
2. Adding I too early
3. Using D without filtering
4. Ignoring dt jitter
5. Blaming PID for sensor glitches
6. Not clamping integrators
7. Testing near ground effect too early

---

## Debug Checklist (When It Feels Wrong)

If behavior is unstable:
1. Confirm sensor health + update rates
2. Confirm dt stability (loop timing)
3. Confirm motor order & direction
4. Confirm mixer signs
5. Confirm estimator outputs
6. Only then adjust PID gains

---

## Suggested Workflow

1. Make one change at a time
2. Commit changes with short notes (gains + result)
3. Save logs with meaningful names:
   - `2025-12-23_altitude_P0.8_D0.05.csv`
4. Keep a tuning journal in `docs/pid_tuning_notes.md`

---

## Final Notes

PID tuning is not magic.
It is an iterative measurement process.

Stable control comes from:
- good estimation
- good timing
- safe limits
- careful tuning

When you get stuck, **log more and change less**.
