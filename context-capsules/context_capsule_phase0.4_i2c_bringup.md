# Context Capsule — StampFly I2C Sensor Bring-Up (Phase 0)

## Project
**StampFly** is a semi-autonomous micro‑drone based on the **M5 StampFly (ESP32‑S3)** platform.

The project follows a strict **phased roadmap** (Phase 0 → Phase 6).
We are **still in Phase 0: hardware bring‑up & confidence**.

This context capsule is intended to be dropped into a **fresh chat window**
to continue work without re‑explaining background.

---

## Phase 0 — Definition

**Goal:**  
Build *confidence* in all hardware components before any estimation,
control, or autonomy logic is added.

**Phase‑0 rules:**
- No sensor fusion
- No control loops
- No filtering beyond what is strictly required for bring‑up
- No “clever” abstractions
- Deterministic, repeatable behavior is king

Exit Phase 0 only when hardware behavior is boring.

---

## Hardware Platform

- MCU: **ESP32‑S3 (M5 StampFly)**
- Firmware: Arduino / PlatformIO
- Buses:
  - **SPI (shared)**  
    - BMI270 IMU  
    - PMW3901 optical flow (now stable)
  - **I2C (next focus)**  
    - Sensors TBD (bring‑up in progress)

---

## SPI Bring‑Up Status (DONE)

### BMI270 IMU
- Official Bosch driver
- Custom SPI glue
- Explicit CS assert/deassert per transaction
- CS idle HIGH enforced in board init
- Robust gyro bias calibration with stillness gating
- Outputs in FRU body frame
- SI units (m/s², rad/s)
- Stable across repeated boots

### PMW3901 Optical Flow
- Bitcraze‑derived init sequence
- Correct SPI read timing (tSRAD delays)
- Motion gating using motion bit (reg 0x02, bit7)
- Raw dx/dy outputs
- SQUAL (surface quality) exposed
- Empirical quality vs height characterized:
  - ~0 cm: poor (≤10)
  - ~5–10 cm: excellent (~100)
  - ~20 cm: usable (~70)
- No regression on shared SPI bus
- Phase‑0 exit criteria satisfied

---

## Critical SPI Lessons (Non‑Negotiable)

1. **All SPI CS pins must be set OUTPUT + HIGH in board init**
2. Each driver:
   - Uses `SPI.beginTransaction()`
   - Explicitly asserts/deasserts *only its own* CS
3. Drivers never manipulate other device CS pins
4. Timing margins matter more than speed in Phase 0

These rules are now considered *hard requirements*.

---

## Current Phase‑0 Task: I2C Sensor Bring‑Up

### Objective
Bring up all **I2C‑connected sensors** with the same discipline used for SPI:

- Deterministic initialization
- Reliable reads
- Correct addressing
- No bus contention
- No impact on existing SPI sensors

### Scope
For each I2C sensor:
- Confirm I2C address
- Verify device ID / WHO_AM_I (if available)
- Read raw data registers
- Validate values are sane at rest
- Observe behavior under simple physical stimulus
- Confirm bus stability under load

### Out of Scope
- Sensor fusion
- Filtering / averaging
- Control loops
- Timing optimization
- Power‑saving modes

---

## Design Expectations for I2C Drivers

- One driver per sensor
- Explicit error handling on init
- No retries hidden inside read paths
- Clear return semantics:
  - `read()` returns false on invalid / stale data
- Expose raw values + health/status flags
- Keep API boring and explicit

---

## Suggested Bring‑Up Pattern (per sensor)

1. **Board init**
   - Confirm I2C bus configured once
   - Pull‑ups verified (hardware or internal)

2. **Begin / Init**
   - Read ID register
   - Fail hard if ID mismatches
   - Log init success/failure

3. **Raw read**
   - Dump raw register values
   - Confirm stability at rest
   - Confirm change with stimulus

4. **Timing**
   - Measure read duration
   - Confirm no blocking / long stalls

5. **Regression check**
   - Ensure SPI sensors still work
   - Ensure other I2C devices still respond

---

## Success Criteria (Phase 0 Exit for I2C)

- All I2C sensors enumerate reliably
- IDs read correctly on every boot
- Raw data behaves physically correctly
- No intermittent bus failures
- No regressions in SPI sensors
- Loop timing remains stable

---

## Architectural Notes

- Phase‑0 drivers live under:
  ```
  src/sensors/
    ├── imu/
    ├── flow/
    └── i2c/
  ```
- `main.cpp` remains boring:
  - init
  - periodic reads
  - debug prints
- No cross‑sensor dependencies in Phase 0

---

## Mindset Reminder

> If a sensor “mostly works”, it does **not** work.

Phase 0 exists to remove uncertainty.
If behavior feels surprising, stop and instrument it.

---

## Next Conversation Prompt (Suggested)

> “We are continuing Phase 0 bring‑up of I2C sensors on StampFly.
> SPI sensors (BMI270 + PMW3901) are stable.
> Please help bring up the first I2C sensor using the same deterministic approach.”

