#pragma once
#include <Arduino.h>
#include <Wire.h>

struct PresSample {
  bool valid = false;
  float temp_c = NAN;
  float press_pa = NAN;
};

class PresBmp280 {
public:
  bool begin(TwoWire& wire, uint8_t addr7 = 0x76);
  void read(PresSample& out);

private:
  bool _ok = false;
};
