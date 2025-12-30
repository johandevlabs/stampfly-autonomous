# Sensors (Umbrella) – Phase 0 Bring-up

This folder provides a **single “umbrella” `Sensors` class** that owns all sensor drivers and exposes
a **cached root sample** (`SensorsSample`) updated by rate-group read functions:

- `Sensors::fast_read()` – **250 Hz** group (SPI-heavy, latency-sensitive)
- `Sensors::slow_read()` – **20 Hz** group (I²C sensors like ToF)
- `Sensors::very_slow_read()` – **1 Hz** group (power monitor, slow housekeeping)

The goal is Phase-0 simple determinism:
- **Each physical sensor is read exactly once in its intended loop rate.**
- The **1 Hz report loop prints cached values only** (no extra SPI/I²C transactions).
- Avoids jitter, duplicate reads, and confusing logs.

---

## File layout

```
src/sensors/
  sensors.h
  sensors.cpp
  README.md
  power/
    power_ina3221.h
    power_ina3221.cpp
    README.md
  imu/
  flow/
  tof/
```

> Note: The exact subfolder names for `imu/`, `flow/`, `tof/` depend on your project; this README
documents the umbrella pattern regardless of driver internals.

---

## Ownership rules

### Read sensors only in the designated loop
- **Fast loop (250 Hz)** calls `fast_read()` (SPI devices like IMU/flow).
- **Slow loop (20 Hz)** calls `slow_read()` (I²C devices like ToF).
- **Very slow loop (1 Hz)** calls `very_slow_read()` (power monitor + slow housekeeping).

### Reporting prints cached values only
The **1 Hz report** must *not* call any driver `read()` functions directly.  
It should print from:

```cpp
const auto& s = sensors.sample();
```

### Never `return;` from `loop()` due to a sensor failure
Sensors may become temporarily unavailable (e.g., INA disappears during VBAT sag).  
Instead:
- set `*_valid = false`
- increment an error counter
- keep running (so timing stats + other sensors continue)

---

## Interfaces

### `SensorsSample`
A single struct containing:
- timestamps for last update per rate-group
- last cached sample per sensor
- validity flags and (optionally) error counters

Example fields (your project’s exact structs may differ):

- `ImuSample imu; bool imu_valid;`
- `FlowSample flow; bool flow_valid;`
- `TofSample tof_down; bool tof_down_valid;`
- `PowerSample power; bool power_valid; uint32_t power_err;`

### `Sensors` class
Minimal interface:

```cpp
bool begin(TwoWire& wire);
void fast_read();
void slow_read();
void very_slow_read();

const SensorsSample& sample() const;
```

No inheritance, no templates, no virtual methods—**Phase-0 simple**.

---

## Loop integration example

```cpp
static Sensors sensors;

void setup() {
  // board_init() should call Wire.begin() already
  bool ok = sensors.begin(Wire);
  Serial.printf("[sensors] begin: %s\n", ok ? "OK" : "FAIL");
}

void loop() {
  if (fast_stats.ready(now, FAST_PERIOD_US)) {
    fast_stats.tick(now);
    sensors.fast_read();
  }

  if (slow_stats.ready(now, SLOW_PERIOD_US)) {
    slow_stats.tick(now);
    sensors.slow_read();
  }

  if (report_stats.ready(now, REPORT_PERIOD_US)) {
    report_stats.tick(now);

    // Only very slow reads here (power)
    sensors.very_slow_read();

    const auto& s = sensors.sample();
    // print cached values only
  }
}
```

---

## Notes on the power monitor (INA3221)

- The INA3221 is slow-rate friendly: read at **1 Hz** by default.
- Because `VPU` is tied to `VBAT_IN`, a strong droop can cause I²C pull-ups to collapse and the INA
  to “disappear”. Treat repeated invalid reads as a useful brownout indicator.

See `src/sensors/power/README.md` for wiring and channel mapping details.

---

## When to extend this pattern (later phases)

Keep Phase-0 minimal. In later phases you can add:
- health flags / stale timers per sensor
- structured telemetry packing from `SensorsSample`
- rate limiting / smoothing (EWMA) for power
- “sensor reset” logic (e.g., re-init ToF on repeated stale states)
- a formal scheduler (only when needed)

For now, the umbrella class is meant to **reduce bus load and increase determinism** while keeping
the code easy to reason about during bring-up.
