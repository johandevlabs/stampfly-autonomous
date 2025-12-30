#pragma once
#include <Arduino.h>
#include <Wire.h>

struct MagSample {
  bool valid = false;
  int16_t x = 0;
  int16_t y = 0;
  int16_t z = 0;
  uint8_t chip_id = 0;
};

class MagBmm150 {
public:
  bool begin(TwoWire& wire, uint8_t addr7 = 0x10);
  void read(MagSample& out);

private:
  TwoWire* _wire = nullptr;
  uint8_t _addr = 0;
  bool _ok = false;

  bool read8(uint8_t reg, uint8_t& v);
  bool readN(uint8_t reg, uint8_t* buf, size_t n);
  bool write8(uint8_t reg, uint8_t v);

  static int16_t unpack13(uint8_t lsb, uint8_t msb); // BMM150 uses 13-bit for X/Y
  static int16_t unpack15(uint8_t lsb, uint8_t msb); // Z is 15-bit
};
