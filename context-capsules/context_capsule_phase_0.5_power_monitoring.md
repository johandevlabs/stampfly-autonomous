# Context Capsule — StampFly Autonomous
## Phase 0 → Power Monitoring Bring-Up

**Date:** 2025-01-XX  
**Author:** Johan

This context capsule summarizes the state of the StampFly Autonomous project at the end of **Phase 0 ToF bring-up** and provides the necessary background to continue with **power monitoring sensor integration** in a fresh chat/session.

---

## Project Overview

- Platform: **M5Stack StampFly**
- MCU: **ESP32-S3 (Stamp-S3)**
- OS/Framework: **Arduino / ESP-IDF via PlatformIO**
- Project goal: Incremental bring-up of sensors and subsystems toward autonomous flight

Phase-0 focuses on **hardware validation, sensor bring-up, and observability**, not flight control or robustness masking.

---

## Sensors Integrated So Far (Phase 0)

### SPI Sensors (Stable)
- **BMI270 IMU**  
  - Fully initialized and streaming
  - Gyro bias calibration implemented
  - Used in fast loop (250 Hz)

- **PMW3901 Optical Flow**  
  - SPI bring-up complete
  - Produces consistent raw motion and quality metrics

### I²C Sensors

#### VL53L3 Time-of-Flight Sensors
- Two sensors on internal I²C bus (G3/G4)

| Sensor | Orientation | Address | Power |
|------|------------|--------|------|
| ToF-1 | Downward | 0x30 | Main 3V3 rail |
| ToF-2 | Forward | 0x29 | VBAT → local regulator (battery PCB) |

**Driver:** `sensors/tof_vl53l3/`

Key behaviors:
- Non-blocking polling-based readout
- Explicit `valid` / `stale` semantics
- Hold-last-good for short dropouts
- Near-field limitation below ~20 mm (expected)

**Known limitations (documented):**
- Forward ToF (ToF-2) may enter a stuck ranging state
- Strong correlation with VBAT / battery condition
- StampFly does NOT charge battery over USB
- Low or absent battery causes ToF-2 instability and I²C errors

Hard reset / XSHUT recovery intentionally NOT used in Phase 0.

---

## Observed System-Level Issues

- I²C errors observed during experiments:
  ```
  requestFrom(): i2cWriteReadNonStop returned Error -1
  ```
- These errors correlate strongly with ToF-2 instability
- Root cause suspected to be **battery voltage too low** and/or regulator dropout

Battery charging requires **external charger or M5 Atom Joystick** (not available).

ToF-2 has been temporarily disabled and battery disconnected to stabilize the system.

---

## Phase 0 Status

- [x] SPI bus stable
- [x] I²C bus stable when VBAT-dependent sensor disabled
- [x] ToF-1 (down) validated on USB-only power
- [x] ToF-2 behavior understood and limitation documented
- [x] README written for `sensors/tof_vl53l3/`

Phase-0 ToF bring-up is considered **closed**.

---

## Next Objective (New Session)

### Phase 0.5 / Phase 1: Power Monitoring

Primary goal:
> **Gain observability into VBAT and 3V3 rails to correlate sensor failures with power events.**

Key questions to answer:
- What is the actual VBAT under load?
- Does VBAT sag during motion / sensor activity?
- Is the front ToF regulator dropping out?
- Are I²C errors correlated with voltage events?

---

## Planned Power Monitoring Work

- Identify available power monitor hardware on StampFly (or external sensor)
- Candidate approaches:
  - ADC + resistor divider for VBAT
  - Dedicated power monitor IC (e.g. INA219 / INA226 / INA3221 if available)
- Define sampling rate and logging strategy
- Integrate power telemetry into slow loop (20 Hz)
- Correlate power data with existing sensor logs

---

## Coding & Architectural Notes

- Sensor objects are owned and initialized in `main.cpp`
- Pin definitions live in `config/pins.h`
- Board-level setup in `src/board/board_init.*`
- Drivers are intentionally simple and explicit (Phase-0 style)

---

## Guidance for Next Chat Session

When starting the next session, assume:
- ToF driver exists and is stable enough for correlation work
- Front ToF may be disabled initially
- Focus is **power visibility**, not ToF recovery
- Avoid masking issues until voltage data is available

---

_End of context capsule._

