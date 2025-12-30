#include "sensors/mag/mag_bmm150.h"

static constexpr uint8_t REG_CHIP_ID      = 0x40; // expect 0x32
static constexpr uint8_t REG_POWER_CTRL   = 0x4B; // write 0x01 to enable
static constexpr uint8_t REG_OPMODE       = 0x4C; // keep simple for now
static constexpr uint8_t REG_DATA_X_LSB   = 0x42; // 0x42..0x47

static constexpr uint8_t CHIP_ID_BMM150   = 0x32;

bool MagBmm150::begin(TwoWire& wire, uint8_t addr7) {
  _wire = &wire;
  _addr = addr7;
  _ok = false;

  // Some boards need a brief settle (you already saw that power ctrl matters)
  delay(2);

  // Enable power control (key step you discovered)
  if (!write8(REG_POWER_CTRL, 0x01)) return false;
  delay(2);

  uint8_t id = 0;
  if (!read8(REG_CHIP_ID, id)) return false;

  // Optional: set opmode to "normal" (0x00 is commonly used for normal in many wrappers;
  // keeping it as-is also works for basic reads on some boards)
  // If you see zeros later, try write8(REG_OPMODE, 0x00);
  (void)write8(REG_OPMODE, 0x00);
  delay(2);

  _ok = (id == CHIP_ID_BMM150);
  return _ok;
}

void MagBmm150::read(MagSample& out) {
  out = MagSample{};
  if (!_ok || !_wire) return;

  uint8_t id = 0;
  if (!read8(REG_CHIP_ID, id)) return;
  out.chip_id = id;
  if (id != CHIP_ID_BMM150) return;

  uint8_t b[6] = {0};
  if (!readN(REG_DATA_X_LSB, b, sizeof(b))) return;

  // X/Y are 13-bit signed, Z is 15-bit signed in the common Bosch format.
  // This is “good enough” for bring-up / non-calibrated heading tests.
  out.x = unpack13(b[0], b[1]);
  out.y = unpack13(b[2], b[3]);
  out.z = unpack15(b[4], b[5]);

  out.valid = true;
}

bool MagBmm150::read8(uint8_t reg, uint8_t& v) {
  if (!_wire) return false;
  _wire->beginTransmission(_addr);
  _wire->write(reg);
  if (_wire->endTransmission(false) != 0) return false; // repeated start
  if (_wire->requestFrom((int)_addr, 1) != 1) return false;
  v = _wire->read();
  return true;
}

bool MagBmm150::readN(uint8_t reg, uint8_t* buf, size_t n) {
  if (!_wire) return false;
  _wire->beginTransmission(_addr);
  _wire->write(reg);
  if (_wire->endTransmission(false) != 0) return false;
  if (_wire->requestFrom((int)_addr, (int)n) != (int)n) return false;
  for (size_t i = 0; i < n; i++) buf[i] = _wire->read();
  return true;
}

bool MagBmm150::write8(uint8_t reg, uint8_t v) {
  if (!_wire) return false;
  _wire->beginTransmission(_addr);
  _wire->write(reg);
  _wire->write(v);
  return (_wire->endTransmission(true) == 0);
}

int16_t MagBmm150::unpack13(uint8_t lsb, uint8_t msb) {
  // [msb:7..0][lsb:7..3] => 13-bit (lsb lower 3 bits are status / unused)
  int16_t raw = (int16_t)((((uint16_t)msb) << 8) | lsb);
  raw >>= 3; // drop status bits
  // sign extend 13-bit
  if (raw & 0x1000) raw |= 0xE000;
  return raw;
}

int16_t MagBmm150::unpack15(uint8_t lsb, uint8_t msb) {
  // Z is often 15-bit with 1 status bit in lsb
  int16_t raw = (int16_t)((((uint16_t)msb) << 8) | lsb);
  raw >>= 1;
  if (raw & 0x4000) raw |= 0x8000;
  return raw;
}
