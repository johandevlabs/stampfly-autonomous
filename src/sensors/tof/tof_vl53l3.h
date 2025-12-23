#pragma once
#include <stdint.h>

struct TofSample {
  float range_cm = 0;
  uint32_t t_us = 0;
  uint8_t status = 0;   // driver-specific status
  bool valid = false;
};

class TofVl53l3 {
 public:
  bool begin(uint8_t i2c_addr);
  bool read(TofSample& out);
};
