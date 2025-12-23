#include "baro_bmp280.h"

bool BaroBmp280::begin(uint8_t i2c_addr) {
  // TODO: init BMP280 at i2c_addr
  (void)i2c_addr;
  return false;
}

bool BaroBmp280::read(BaroSample& out) {
  // TODO: read pressure/temp
  (void)out;
  return false;
}
