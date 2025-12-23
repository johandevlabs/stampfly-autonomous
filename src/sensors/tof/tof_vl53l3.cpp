#include "tof_vl53l3.h"

bool TofVl53l3::begin(uint8_t i2c_addr) {
  // TODO: init VL53L3 at i2c_addr
  (void)i2c_addr;
  return false;
}

bool TofVl53l3::read(TofSample& out) {
  // TODO: read range
  (void)out;
  return false;
}
