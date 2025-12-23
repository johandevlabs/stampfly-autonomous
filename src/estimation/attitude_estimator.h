#pragma once
#include <stdint.h>

struct AttitudeState {
  float roll_deg = 0, pitch_deg = 0, yaw_deg = 0;
  uint32_t t_us = 0;
  bool valid = false;
};

class AttitudeEstimator {
 public:
  bool begin();
  void update(float gx_dps, float gy_dps, float gz_dps,
              float ax_g, float ay_g, float az_g,
              float dt_s);
  AttitudeState state() const;

 private:
  AttitudeState s_;
};
