#pragma once
#include <stdint.h>

struct FlowSample {
  float dx = 0;         // raw flow units (sensor-specific)
  float dy = 0;
  uint32_t t_us = 0;
  bool valid = false;
};

class FlowPmw3901 {
 public:
  bool begin();
  bool read(FlowSample& out);
};
