# Context Capsule — StampFly PMW3901 Bring-Up (Phase 0)

## Project
We are developing a **semi-autonomous drone** based on the **M5 StampFly (ESP32-S3)** platform.
The project follows a phased roadmap (Phase 0 → Phase 6).
We are **strictly in Phase 0 (hardware bring-up & confidence)**.

## Phase 0 Goals
- Enumerate all sensors
- Verify reliable communication
- Verify update rates & timing
- Ensure shared buses (SPI) are robust
- No estimation, control, or autonomy logic yet

## Hardware
- MCU: ESP32-S3 (Arduino / PlatformIO)
- Shared SPI bus:
  - **BMI270 IMU**
  - **PMW3901 optical flow sensor**
- Dedicated CS pin per SPI device

## Current Status — BMI270 (STABLE)
- Bosch official BMI2 / BMI270 driver
- Custom Arduino SPI glue
- Correct SPI dummy-byte handling
- Explicit CS assert/deassert per transaction
- µs-scale CS timing guards
- Conservative init timing
- Robust gyro bias calibration with stillness gating
- Outputs in **FRU body frame** (Forward, Right, Up)
- Scaled SI units (m/s², rad/s)

### Key Lesson Learned
Intermittent BMI270 init failures were caused by **SPI bus contention**:
- PMW3901 CS pin was floating at boot
- Fix: **ALL SPI CS pins must be set OUTPUT/HIGH in board init**

This is now considered a hard requirement.

## Current Task — PMW3901 Bring-Up (Phase 0)
Objective: **prove reliable PMW3901 communication** without impacting BMI270.

Scope is intentionally limited to:
- SPI communication reliability
- Correct CS behavior
- Reading device ID / known registers
- Reading raw `dx`, `dy`, and quality values
- Verifying stable output at rest and during motion
- Verifying no regressions in BMI270 behavior

## Out of Scope (for this phase)
- Sensor fusion
- Velocity estimation
- Position hold
- Control loops
- Autonomy logic

## Design Expectations
- PMW3901 driver must:
  - Use SPI transactions
  - Explicitly assert/deassert CS
  - Leave CS HIGH when idle
- No sensor driver should manipulate other device CS pins
- SPI bus idling belongs in **board init**, not drivers
- Focus on deterministic, repeatable behavior

## Success Criteria
- Device ID reads correctly and consistently
- `dx/dy ≈ 0` at rest
- `dx/dy` responds correctly to motion
- No BMI270 init failures
- Stable read timing (measured, not assumed)
