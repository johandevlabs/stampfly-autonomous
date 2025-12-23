#pragma once
#include <stdint.h>

struct AltitudeState {
  float z_cm = 0;
  float z_dot_cm_s = 0;
  uint32_t t_us = 0;
  bool valid = false;
};

class AltitudeEstimator {
 public:
  bool begin();
  void update_tof(float range_cm, float dt_s);
  void update_baro(float pressure_pa, float dt_s);
  AltitudeState state() const;

 private:
  AltitudeState s_;
};
