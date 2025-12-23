#pragma once
#include <stdint.h>

struct BaroSample {
  float pressure_pa = 0;
  float temperature_c = 0;
  uint32_t t_us = 0;
  bool valid = false;
};

class BaroBmp280 {
 public:
  bool begin(uint8_t i2c_addr);
  bool read(BaroSample& out);
};
