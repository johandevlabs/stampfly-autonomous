# IMU – BMI270 (StampFly)

This folder contains the **BMI270 IMU driver integration** for the StampFly-based semi-autonomous drone platform.

The implementation is based on **Bosch’s official BMI2 / BMI270 driver**, with a thin Arduino/ESP32 SPI glue layer and a project-specific wrapper that provides:

- Robust SPI bring-up on ESP32-S3
- Deterministic initialization
- Gyro bias calibration with stillness gating
- Output in **FRU body frame** (Forward, Right, Up)
- Scaled SI units (m/s², rad/s)
- Clean separation between sensor-frame and body-frame logic

This module is intended to be the **single source of truth** for IMU data used by estimation and control loops.

---

## File Overview

```
imu/
├── imu_bmi270.h
├── imu_bmi270.cpp
├── bmi270_bosch_glue.h
├── bmi270_bosch_glue.cpp
└── README.md
```

### imu_bmi270.h / .cpp
High-level BMI270 driver wrapper used by the rest of the system.

Responsibilities:
- Device initialization and retry logic
- Sensor configuration (ODR, range, bandwidth)
- Gyro bias calibration with motion rejection
- Scaling raw data to SI units
- Mapping sensor axes → **FRU body frame**
- Providing a stable `readFRU()` API for control/estimation

### bmi270_bosch_glue.h / .cpp
Low-level SPI glue layer between:
- Bosch BMI2/BMI270 C driver
- Arduino SPI API on ESP32-S3

Responsibilities:
- SPI read/write callbacks required by Bosch driver
- Correct dummy-byte handling for BMI270 SPI
- Proper SPI transactions and CS timing
- No sensor logic, scaling, or frame assumptions

---

## Coordinate Frames

### Sensor Frame
The native BMI270 sensor axes as wired on the StampFly PCB.

This frame is **not exposed** outside the IMU module.

### Body Frame (FRU)
All public IMU data is provided in **FRU**:

- **X**: Forward  
- **Y**: Right  
- **Z**: Up  

Axis mapping (sensor → body):

```
Body X (forward) = Sensor Y
Body Y (right)   = Sensor X
Body Z (up)      = Sensor Z
```

---

## Public API

### Initialization

```cpp
ImuBmi270 imu;
imu.begin();
```

### Reading IMU Data (FRU)

```cpp
ImuSample sample;
if (imu.readFRU(sample)) {
    // sample.ax, ay, az  -> m/s^2 (FRU)
    // sample.gx, gy, gz  -> rad/s  (FRU)
}
```

---

## Gyro Bias Calibration

Gyro bias is estimated during startup using a **stillness-gated averaging** process.

If insufficient still samples are detected:
- Calibration is rejected
- The previous bias is kept (or zero if first boot)

---

## Design Principles

- Single responsibility per layer
- No frame conversions outside IMU
- Deterministic startup behavior
- Fail-safe bias calibration

---

## Known Assumptions

- IMU is BMI270 on SPI
- ESP32-S3 platform
- SPI bus initialized before `begin()`
- Body frame = FRU
