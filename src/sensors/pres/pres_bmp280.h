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
  TwoWire* _wire = nullptr;
  uint8_t _addr = 0;
  bool _ok = false;

  // Calibration (BMP280)
  uint16_t dig_T1 = 0;
  int16_t  dig_T2 = 0, dig_T3 = 0;
  uint16_t dig_P1 = 0;
  int16_t  dig_P2 = 0, dig_P3 = 0, dig_P4 = 0, dig_P5 = 0, dig_P6 = 0, dig_P7 = 0, dig_P8 = 0, dig_P9 = 0;

  int32_t _t_fine = 0;

  bool read8(uint8_t reg, uint8_t& v);
  bool readN(uint8_t reg, uint8_t* buf, size_t n);
  bool write8(uint8_t reg, uint8_t v);

  static uint16_t u16_le(const uint8_t* b) { return (uint16_t)b[0] | ((uint16_t)b[1] << 8); }
  static int16_t  i16_le(const uint8_t* b) { return (int16_t)u16_le(b); }

  bool read_calibration();
  bool configure();
  bool read_raw(int32_t& adc_T, int32_t& adc_P);

  int32_t compensate_T_x100(int32_t adc_T);     // returns temperature in 0.01°C
  uint32_t compensate_P_Q24_8(int32_t adc_P);   // returns pressure in Pa (Q24.8 per Bosch algo)
};
