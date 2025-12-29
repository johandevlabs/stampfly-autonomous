# VL53L3 ToF Sensor Driver (Phase 0)

This module provides a minimal, non-blocking bring-up driver for the **ST VL53L3** time-of-flight distance sensors used on the **StampFly** platform.

StampFly contains **two VL53L3 sensors** on the internal I²C bus:
- **Downward-facing ToF** (ToF-1, bottom of frame)
- **Forward-facing ToF** (ToF-2, front of frame)

This driver is designed for **Phase 0 bring-up**: validation of wiring, power, addressing, and basic ranging behavior. It intentionally avoids aggressive recovery or masking of hardware issues.

---

## Hardware Context

| Sensor | Orientation | I²C Address | Power Source |
|------|------------|------------|-------------|
| ToF-1 | Downward | `0x30` | Main 3V3 rail |
| ToF-2 | Forward | `0x29` | VBAT → local regulator (battery PCB) |

Each sensor exposes:
- `XSHUT` (used for address assignment and reset)
- `GPIO1` (interrupt, not used in Phase 0)

---

## Driver Design Goals (Phase 0)

- Non-blocking readout (polling-based)
- Explicit validity signaling (`valid`, `stale`)
- Graceful handling of temporary dropouts
- Clear separation between:
  - **fresh valid data**
  - **held last-good data**
  - **no usable data**
- No hidden retries or hard resets

---

## Public API

### Data Structure

```cpp
struct TofSample {
  uint32_t t_us;          // Timestamp (micros)
  bool     valid;         // True if usable (fresh or stale)
  bool     stale;         // True if last-good is being held
  uint16_t range_mm;      // Distance in mm (UINT16_MAX if invalid)
  uint8_t  range_status;  // Raw VL53 status (0 = valid)
  uint8_t  stream_count;  // Debug
  uint16_t ambient;       // Optional debug
  uint16_t signal;        // Optional debug
};
```

### Usage Pattern

```cpp
TofSample sample;
tof.read(sample);

if (sample.valid) {
  // Use sample.range_mm
} else {
  // No usable data this cycle
}
```

Notes:
- `range_mm == UINT16_MAX` indicates **invalid / no data**
- Callers **must check `valid`** before using the range

---

## Ranging Behavior

### Valid operating region
- Reliable measurements for **distances ≥ ~20 mm**
- Works from close indoor ranges up to several meters

### Near-field behavior
- For **d < ~20 mm**, the sensor often stops producing valid frames
- This is expected and handled by:
  - holding last-good briefly (configurable)
  - then marking output invalid

---

## Known Limitations (Phase 0)

### 1. Forward ToF (ToF-2) depends on battery voltage

The forward-facing ToF sensor is powered from **VBAT via a local regulator on the front/battery PCB**.

Important consequences:
- The StampFly **does not charge the battery over USB**
- Battery charging must be done externally (e.g. via M5 Atom Joystick or dedicated charger)
- If the battery is low or absent:
  - ToF-2 may fail to power correctly
  - The sensor may partially brown-out while still attached to I²C

This can result in:
- ToF-2 producing no valid ranging data
- Intermittent or permanent loss of front ToF measurements
- I²C transaction errors (e.g. `i2cWriteReadNonStop returned Error -1`)

---

### 2. Forward ToF can enter a "stuck" ranging state

Observed behavior:
- Sensor continues to ACK on I²C
- No new measurement frames become ready
- Downward ToF continues working normally

Likely contributing factors:
- Power instability on VBAT / local regulator
- Loss of optical target (angle, reflectivity, near-field)

Current handling (Phase 0):
- Last-good value may be held briefly
- After timeout, output becomes invalid
- **No automatic hard reset or power-cycling is performed**

This behavior is intentionally not masked and will be revisited once power monitoring is available.

---

### 3. No automatic hard recovery in Phase 0

This driver intentionally avoids:
- XSHUT-based power-cycling during runtime
- Automatic re-initialization loops

Rationale:
- Avoid hiding underlying power integrity issues
- Keep timing behavior deterministic
- Support root-cause analysis during bring-up

Hard recovery strategies may be introduced in later phases if justified.

---

## Phase-0 Acceptance Criteria

- [x] Both sensors enumerate on I²C
- [x] Dual-address configuration works reliably
- [x] Valid ranging data observed in expected distance ranges
- [x] Graceful handling of dropouts
- [x] Known power-related limitations clearly identified

---

## Planned Follow-Up (Future Phases)

- VBAT and 3V3 rail monitoring
- Correlation of ToF dropouts with voltage events
- Re-evaluation of recovery strategies
- Integration with height and obstacle estimation logic

---

## Notes

This module is **bring-up focused**, not a final flight-ready ToF driver.

Design favors observability and correctness over robustness masking.
