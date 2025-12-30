#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <INA3221.h>

struct PowerSample {
  bool     valid = false;
  uint32_t t_ms = 0;

  float vbat_in_v = NAN;  // INA3221 bus voltage on CH2 (VBAT_IN)
  float ishunt_a  = NAN;  // current through shunt (positive means VBAT->load)
  float p_w       = NAN;  // v * i
};

class PowerINA3221 {
public:
  PowerINA3221() = default;
  ~PowerINA3221();

  // addr should be 0x40 in your case
  bool begin(TwoWire& wire, uint8_t addr, float shunt_ohms_ch2);

  PowerSample read();        // read CH2 only (library channel index 1)
  uint32_t errorCount() const { return _err; }
  bool isReady() const { return _ina != nullptr && _ready; }

private:
  INA3221* _ina = nullptr;
  bool _ready = false;

  uint8_t _addr = 0x40;
  float _rshunt = 0.01f;     // Ohms (CH2 only)
  uint32_t _err = 0;

  static constexpr uint8_t CH2 = 1;  // library channel index for INA "CH2"
};
