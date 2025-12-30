#pragma once

#include <Arduino.h>
#include <Wire.h>

#include "config/pins.h"

// Drivers
#include "sensors/imu/imu_bmi270.h"
#include "sensors/flow/flow_pmw3901.h"
#include "sensors/tof/tof_vl53l3.h"
#include "sensors/power/power_ina3221.h"

struct SensorsSample {
  // Timestamps for when each rate-group last updated
  uint32_t t_fast_ms = 0;
  uint32_t t_slow_ms = 0;
  uint32_t t_very_slow_ms = 0;

  // IMU
  bool imu_valid = false;
  ImuSample imu{};

  // Flow
  bool flow_valid = false;
  FlowSample flow{};

  // ToF (down)
  bool tof_down_valid = false;
  TofSample tof_down{};

  // ToF (front)
  bool tof_front_valid = false;
  TofSample tof_front{};

  // Power
  bool power_valid = false;
  PowerSample power{};
  uint32_t power_err = 0;
};

class Sensors {
public:
  Sensors() = default;

  bool begin(TwoWire& wire);     // init all sensors
  void fast_read();              // 250 Hz group
  void slow_read();              // 20 Hz group
  void very_slow_read();         // 1 Hz group (power, etc.)

  const SensorsSample& sample() const { return _s; }

  void printSample() const;

private:
  SensorsSample _s;

  // Concrete drivers
  ImuBmi270 _imu;
  FlowPmw3901 _flow;
  TofVl53L3 _tof_down;
  TofVl53L3 _tof_front;          // optional future

  PowerINA3221 _power;

  // ToF pins (constructed from your pin defines)
  TofPins _downPins{PIN_TOF1_XSHUT, PIN_TOF1_GPIO1};
  TofPins _frontPins{PIN_TOF2_XSHUT, PIN_TOF2_GPIO1};

  bool _power_ok = false;
};
