#pragma once
#include <stdint.h>

/*
quality >= 80 → excellent
quality 30..80 → usable
quality < 30 → poor / don’t trust
*/

struct FlowSample {
  float dx = 0;        // raw flow units (sensor-specific)
  float dy = 0;
  uint8_t motion = 0;
  uint8_t quality = 0; // 0..255-ish, sensor-specific
  bool quality_ok = false;
  uint32_t t_us = 0;
  bool valid = false;
};

class FlowPmw3901 {
 public:
  bool begin();
  bool read(FlowSample& out);
};
