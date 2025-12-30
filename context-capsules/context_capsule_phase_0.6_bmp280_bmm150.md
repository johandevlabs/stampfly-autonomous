# Context Capsule — StampFly Autonomous — Phase 0.x (Sensors Umbrella + Power Monitor complete)
**Date:** 2025-12-30 (Europe/Copenhagen)  
**Branch/PR status:** Work was done on a separate branch, PR merged into `main`.

## Where we are in the roadmap
We are still in **Phase 0 bring-up**, now with a much cleaner architecture for sensor readout and reporting.
Power monitoring via INA3221 is successfully brought up and integrated.

---

## Key outcomes since last capsule

### 1) INA3221 power monitor bring-up (success)
- I2C scan detects: `0x10, 0x30, 0x40, 0x76`
- INA3221 is at **0x40** and initializes successfully (`begin: OK`)
- Power output examples:
  - `VBAT_IN ~1.63V` when not properly powered / very low rail
  - `VBAT_IN ~2.59V` when battery connected (battery suspected flat)
- Current reading sits around `0.004A` at no-load, likely quantization floor / noise (acceptable for bring-up).
- Battery is believed to be flat; a multimeter check of VBAT and VBAT_IN is planned later.

### 2) Introduced Sensors “umbrella” (root sensor manager)
To improve determinism and avoid duplicate sensor reads across loops, a single `Sensors` class now:
- Owns the sensor drivers
- Owns a single cached root struct `SensorsSample`
- Implements rate-grouped read functions:
  - `fast_read()` (250 Hz): IMU + flow (SPI-heavy)
  - `slow_read()` (20 Hz): ToF (I2C)
  - `very_slow_read()` (1 Hz): INA3221 power monitor (I2C)
- Ensures the **1 Hz reporting loop prints cached values only** (no extra SPI/I2C reads).

### 3) Cleaner bring-up logging and reporting
- Each `*.begin()` in `Sensors::begin()` returns `bool`.
- `Sensors::begin()` prints a per-sensor status line: OK/FAIL.
- A `Sensors::printSample()` helper was added to keep `main.cpp` tidy:
  - Prints IMU/Flow/ToF/Power using cached data and validity flags.
  - No `return;` from `loop()` on sensor failure — failures mark samples invalid and continue operation.

### 4) Repository structure / files touched (high level)
- Added/updated:
  - `src/sensors/sensors.h`
  - `src/sensors/sensors.cpp`
  - `src/sensors/README.md` (umbrella pattern documentation; generated as README_sensors.md during drafting)
- Power monitor module location:
  - `src/sensors/power/power_ina3221.h`
  - `src/sensors/power/power_ina3221.cpp`
  - `src/sensors/power/README.md`
- `main.cpp` was refactored to:
  - schedule reads (fast/slow/report)
  - call `Sensors::fast_read()/slow_read()/very_slow_read()`
  - call `Sensors::printSample()` in the report loop

### 5) PlatformIO dependency added
- Added Arduino library:
  - `robtillaart/INA3221@^0.4.1`

---

## Current sensor set (as of now)
- **IMU:** BMI270 (SPI)
- **Flow:** PMW3901 (SPI)
- **ToF (down):** VL53L3 (I2C)
- **Power:** INA3221 (I2C, addr 0x40)
- **I2C scan:** 0x10, 0x30, 0x40, 0x76

---

## Known observations / hypotheses
- INA bus voltage readings are far below normal LiPo levels; battery likely flat.
- With VPU tied to VBAT_IN, hard droops may cause INA to vanish from I2C; treat repeated invalid reads as a useful brownout signal.
- ToF occasionally prints stale or invalid when power conditions change; correlating ToF failures vs VBAT_IN is now straightforward with the current architecture.

---

## Next steps (proposed): Bring up BMP280 + BMM150
The next bring-up targets are:
1) **BMP280** (barometer)  
   - I2C scan already shows `0x76`, which is a very common BMP280/BME280 address.
   - Goal: confirm the device identity and read pressure/temperature reliably.

2) **BMM150** (magnetometer)  
   - Goal: detect on I2C, confirm chip ID, read magnetic field axes.
   - Integrate into `Sensors` umbrella:
     - Likely add to `slow_read()` (20 Hz) or `very_slow_read()` initially, depending on stability.

### Integration approach (consistent with Phase 0)
- Add drivers under `src/sensors/bmp280/` and `src/sensors/bmm150/` (or existing sensor folder conventions).
- Extend `SensorsSample` with:
  - `bmp_valid`, `BmpSample bmp`
  - `mag_valid`, `MagSample mag`
- Extend `Sensors::begin()`:
  - `bmp_ok = _bmp.begin(...); print OK/FAIL`
  - `mag_ok = _mag.begin(...); print OK/FAIL`
- Extend `Sensors::slow_read()` (recommended) to cache samples.
- Extend `Sensors::printSample()` to print pressure/temp + mag axes.

---

## Quick checklist for next session
- [ ] Identify which device responds at `0x76` (BMP280 vs BME280 vs other)
- [ ] Confirm BMM150 I2C address and detection (scan may reveal a new address once wired/enabled)
- [ ] Add minimal begin/read/print integration into Sensors umbrella
- [ ] Keep reads single-owner by rate; report loop prints cached only
- [ ] Validate stability (timing stats stay sane, no I2C spam)

---

## Notes for whoever continues (future-you)
- Keep Phase 0 philosophy: boring, explicit, deterministic.
- Prefer “OK/FAIL” bring-up lines over silent failures.
- Don’t add generic inheritance layers yet; the umbrella class is enough for now.
