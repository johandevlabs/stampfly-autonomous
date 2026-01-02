# Context Capsule — StampFly Autonomous — Phase 0.7 (LED + Buzzer Bring-up)
**Date:** 2026-01-02 (Europe/Copenhagen)  
**Project:** `stampfly-autonomous`  
**Current status:** Phase 0 bring-up in progress; sensors are integrated and stable enough to proceed with indicators.

---

## Where we are in the roadmap
We are still in **Phase 0 — Bring-up & Hardware Confidence**.

Phase 0 exit criteria require:
- All sensors read reliably ✅ (now complete)
- Motors respond correctly ⏳
- LED and buzzer tests ⏳
- No brown-outs/unexpected resets ⏳

We decided to finish Phase 0 in three remaining sub-phases:
- **Phase 0.7:** LED + buzzer
- **Phase 0.8:** Individual motor spin test
- **Phase 0.9:** Phase-0 “confidence routine” + Phase 0 exit

---

## Current sensor bring-up (complete)
Confirmed via I2C scan and chip IDs:
- **BMM150 magnetometer** @ `0x10` (chip ID `0x32`)
  - Important: BMM150 requires explicit power-on: write `0x01` to reg `0x4B` before valid chip ID/data.
- **BMP280 barometer** @ `0x76` (chip ID `0x58`)
  - Implemented as **register-level driver** (Bosch compensation), no Adafruit dependencies.

I2C scan shows: `0x10, 0x30, 0x40, 0x76`  
(0x40 = INA3221 power monitor; 0x30 likely ToF; 0x76 BMP280; 0x10 BMM150)

Sensors umbrella (`src/sensors/sensors.*`) owns all reads + caching:
- `fast_read()` (250 Hz): IMU (BMI270) + optical flow (PMW3901)
- `slow_read()` (20 Hz): ToF (VL53L3 “down”)
- `very_slow_read()` (1 Hz): Power (INA3221) + Pressure (BMP280) + Magnetometer (BMM150)
- `Sensors::printSample()` prints **cached** values only, driven by a 1 Hz report loop

Observed logs (stationary test):
- BMP280 stable ~28–29°C and ~101.85 kPa
- BMM150 raw XYZ changes plausibly
- ToF works when there is clearance; may be invalid on soft surfaces (sofa)
- Flow may be invalid without suitable textured surface; becomes valid when conditions are right

Known issue (to address later, separate commit): `fast_read()` average period still above 4 ms target (was improved by removing `delay(1)` in loop). Timing work is explicitly deferred until after the LED/buzzer commit.

---

## Power observations (known constraint)
INA3221 indicates **very low VBAT_IN ~1.63–1.65 V** (battery likely flat / power domain constraints).
This is treated as “known hardware constraint” for now.
Phase 0 confidence routine (0.9) should correlate:
- sensor validity
- INA readings
- reset reasons / brownouts

---

## Phase 0.7 goal: LED + Buzzer bring-up
### Purpose
Provide a **non-serial** way to signal states/faults and to support later phases (arming, failsafe, low battery).

### Implementation decisions
- Add indicators module in: `src/board/indicators/`
  - `indicators.h`
  - `indicators.cpp`
  - `README.md`
- Add a **compile-time bring-up switch**:
  - `BRINGUP_MODE` (enum or macros)
  - Modes planned:
    - `SENSORS_ONLY` (existing behavior)
    - `INDICATORS_TEST` (Phase 0.7)
    - `MOTOR_TEST` (Phase 0.8)
    - `PHASE0_CONFIDENCE` (Phase 0.9)

### Phase 0.7 deliverables
1) `Indicators::begin()` initializes LED + buzzer GPIO/PWM cleanly.
2) LED patterns:
   - solid ON
   - slow blink
   - fast blink
   - “SOS” or simple pattern (optional)
3) Buzzer patterns:
   - short beep
   - double beep
   - long beep
4) A bring-up demo loop (`BRINGUP_MODE=INDICATORS_TEST`) that cycles:
   - LED patterns (each for ~1–2 seconds)
   - buzzer patterns (each separated by pauses)
5) No dynamic memory; no delays that would later interfere with control loops (use millis-based timing).

### Acceptance criteria (Phase 0.7)
- LED patterns are clearly visible and repeatable across resets.
- Buzzer beeps are clearly audible and repeatable across resets.
- No unexpected resets during indicator cycling.

---

## Pin mapping / config (TODO)
We need to confirm where pin constants live and what pins to use:
- LED pin(s): __TBD__
- Buzzer pin: __TBD__
- Any required PWM channel/timer mapping: __TBD__

Preferred approach:
- Put pin constants in `src/config/` (or existing config location)
- `Indicators` module uses config constants, not literals

---

## Next phases (after 0.7)
### Phase 0.8: Motor test
- Individual motor spin (one at a time) with low duty + ramp.
- Confirms motor mapping and that PWM -> motor works.
- Must be safe and easy to stop (disarm/kill pattern, or compile-time mode that never arms by accident).

### Phase 0.9: Phase-0 confidence routine + exit
- A repeatable “confidence run” combining:
  - sensor readout stability
  - indicator signals
  - motor burst tests
  - monitoring for brownouts/resets
  - logging reset reason at boot + INA readings during load

---

## File/Folder snapshot (relevant)
- `src/sensors/` umbrella and drivers exist for:
  - IMU (BMI270), Flow (PMW3901), ToF (VL53L3), Power (INA3221), Pressure (BMP280), Mag (BMM150)
- New to add in Phase 0.7:
  - `src/board/indicators/indicators.h`
  - `src/board/indicators/indicators.cpp`
  - `src/board/indicators/README.md`
  - `BRINGUP_MODE` compile-time switch (location TBD; likely `src/config/bringup_mode.h` or similar)

---

## Notes for the next chat session (what to ask Aurix)
- Propose a clean `Indicators` API (begin, setLedPattern, beepPattern, update tick).
- Decide best place for `BRINGUP_MODE` define and how to structure modes without cluttering `main.cpp`.
- Confirm LED/buzzer pin mapping for StampFly (from existing board config in repo).
- Implement non-blocking demo sequence for INDICATORS_TEST mode.
