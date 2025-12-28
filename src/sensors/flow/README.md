# flow_pmw3901 — PMW3901 Optical Flow Driver (StampFly / ESP32-S3)

This library provides a **Phase 0 / bring-up-grade** driver for the **PMW3901 optical flow sensor** on the StampFly project.

It focuses on:
- **robust SPI communication**
- **repeatable initialization**
- reading **raw dx/dy motion deltas**
- exposing **motion status** and **surface quality (SQUAL)**

It intentionally does **not** do:
- velocity scaling (cm/s)
- altitude compensation
- filtering / fusion
- control-loop integration

---

## Hardware / SPI assumptions

- Shared SPI bus with other devices (e.g., BMI270 IMU).
- PMW3901 is on its own chip-select pin.

**Hard requirement (board-level init):**
- All SPI CS pins must be configured as `OUTPUT` and driven **HIGH** at boot to avoid bus contention.

---

## Files

- `flow_pmw3901.h` — API + `FlowSample`
- `flow_pmw3901.cpp` — driver implementation (Bitcraze-derived init)

---

## API

### `bool FlowPmw3901::begin()`

Initializes the PMW3901.

What it does:
- Resets the SPI “CS state” with a short HIGH/LOW/HIGH wiggle
- Issues **Power-On Reset** (`0x3A = 0x5A`)
- Verifies communication by reading:
  - Product ID: `0x00 == 0x49`
  - Inverse ID: `0x5F == 0xB6`
- Reads motion regs once (priming)
- Applies Bitcraze “performance optimization” register script

Returns:
- `true` on success
- `false` if ID checks fail

### `bool FlowPmw3901::read(FlowSample& out)`

Reads a single optical-flow sample **if a new sample is available**.

Semantics:
- Always updates:
  - `out.t_us` (timestamp)
  - `out.motion` (reg `0x02`)
  - `out.quality` (SQUAL, reg `0x07`)
  - `out.quality_ok` (`quality >= 30`)
- Uses a **motion gate**:
  - If `(motion & 0x80) == 0`: there is **no new motion sample**
    - sets `out.valid=false`
    - returns `false`
  - Else:
    - reads dx/dy registers (`0x03..0x06`)
    - sets `out.valid=true`
    - returns `true`

### `FlowSample`

```cpp
struct FlowSample {
  float dx = 0;        // raw flow units (sensor-specific)
  float dy = 0;
  uint8_t motion = 0;  // motion/status register (0x02)
  uint8_t quality = 0; // SQUAL (0x07)
  bool quality_ok = false;
  uint32_t t_us = 0;   // timestamp (micros)
  bool valid = false;  // true when dx/dy were updated
};
```

---

## Example usage

```cpp
#include "flow_pmw3901.h"

FlowPmw3901 g_flow;

void setup() {
  Serial.begin(115200);

  if (!g_flow.begin()) {
    Serial.println("[flow] PMW3901 init failed!");
    while (1) delay(100);
  }
  Serial.println("[flow] PMW3901 init OK");
}

void loop() {
  FlowSample fs;
  if (g_flow.read(fs)) {
    Serial.printf("[flow] dx=%.0f dy=%.0f motion=0x%02X q=%u qok=%d\n",
                  fs.dx, fs.dy, (unsigned)fs.motion,
                  (unsigned)fs.quality, fs.quality_ok);
  }
}
```

---

## Interpreting `quality` (SQUAL)

`quality` is a proxy for “how well the sensor can track the surface”.

Empirical bring-up measurements (rough):
- **0–1 cm above surface:** quality ~ **≤10** (often too close / out of focus)
- **~5–10 cm:** quality ~ **~100** (sweet spot)
- **~20 cm:** quality ~ **~70** (still usable)

Suggested interpretation:
- `quality >= 80` → excellent
- `quality 30..80` → usable
- `quality < 30` → poor / don’t trust

Note: surface texture + lighting matter a lot (textured matte surfaces track best).

---

## Notes on coordinate frames

`dx/dy` are **raw sensor axes**. They are not yet mapped into the drone FRU frame.
Do the axis/sign mapping in a higher layer once sensor orientation is finalized.

---

## Troubleshooting checklist

If `begin()` fails (ID reads wrong / 0xFF):
1. Verify PMW3901 power (3.3V) and SPI wiring (MISO/MOSI/SCK/CS).
2. Ensure CS pins are `OUTPUT/HIGH` in board init.
3. Ensure the PMW3901 read timing includes the required small delays (tSRAD).
4. Confirm no other SPI device is driving MISO during PMW3901 CS low.

If `dx/dy` never change:
- Ensure the Bitcraze init script is applied.
- Test at 5–10 cm above a textured surface with good lighting.

---

## License / attribution

Initialization sequence and timing behavior are adapted from the Bitcraze PMW3901 implementation
(commonly used in the Crazyflie ecosystem). Keep their license header if you copy/paste their tables verbatim.

---

## Phase roadmap context

This driver is meant for **Phase 0 (Bring-up & Hardware Confidence)**:
- prove reliable communication
- characterize output and quality behavior
- avoid regressions on the shared SPI bus
