#pragma once
#include <stdint.h>

struct ImuSample {
  float gx_dps = 0, gy_dps = 0, gz_dps = 0;
  float ax_g = 0, ay_g = 0, az_g = 0;
  uint32_t t_us = 0;
  bool valid = false;
};

class ImuBmi270 {
 public:
  bool begin();
  bool read(ImuSample& out);
};
