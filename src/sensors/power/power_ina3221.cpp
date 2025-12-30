#include "power_ina3221.h"

PowerINA3221::~PowerINA3221() {
  if (_ina) {
    delete _ina;
    _ina = nullptr;
  }
}

bool PowerINA3221::begin(TwoWire& wire, uint8_t addr, float shunt_ohms_ch2) {
  _addr = addr;
  _rshunt = shunt_ohms_ch2;
  _err = 0;
  _ready = false;

  if (_ina) {
    delete _ina;
    _ina = nullptr;
  }

  // NOTE: Wire.begin() is already done in your board_init()
  _ina = new INA3221(_addr, &wire);

  if (!_ina->begin()) {
    _err++;
    delete _ina;
    _ina = nullptr;
    return false;
  }

  // Only CH2 is wired. Disable CH1 & CH3 to keep background conversion loop clean.
  _ina->disableChannel(0);
  _ina->enableChannel(CH2);
  _ina->disableChannel(2);

  // Set shunt resistor for CH2 so current/power become meaningful.
  // Returns 0 on success. (Per library README.)
  int rc = _ina->setShuntR(CH2, _rshunt);
  if (rc != 0) {
    _err++;
    // still usable for voltage; but mark not-ready to force attention
    _ready = false;
    return false;
  }

  // Optional: leave defaults unless you want slower / smoother readings:
  // _ina->setAverage(4); // 128 samples (slow, smooth)
  // _ina->setModeShuntBusContinuous(); // default

  _ready = true;
  return true;
}

PowerSample PowerINA3221::read() {
  PowerSample s;
  s.t_ms = millis();

  if (!_ina || !_ready) {
    s.valid = false;
    return s;
  }

  // If VBAT_IN collapses, the chip may "disappear" from I2C — treat that as signal.
  if (!_ina->isConnected()) {
    _err++;
    s.valid = false;
    return s;
  }

  const float v = _ina->getBusVoltage(CH2);
  const float i = _ina->getCurrent(CH2);
  const float p = _ina->getPower(CH2);

  // Basic sanity: a negative here could happen if polarity is swapped,
  // or during regen/backfeed edge cases. We keep it as-is.
  s.vbat_in_v = v;
  s.ishunt_a  = i;
  s.p_w       = p;
  s.valid = true;

  return s;
}
