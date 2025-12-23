#pragma once
#include <stdint.h>

enum class FlightMode : uint8_t {
  IDLE = 0,
  ALT_HOLD = 1,
  VEL_HOLD = 2,
  LAND = 3,
  FAILSAFE = 4
};
