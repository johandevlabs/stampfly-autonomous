#include "sensors/pres/pres_bmp280.h"

// BMP280 registers
static constexpr uint8_t REG_ID        = 0xD0; // chip id (0x58)
static constexpr uint8_t REG_RESET     = 0xE0; // reset (0xB6)
static constexpr uint8_t REG_STATUS    = 0xF3;
static constexpr uint8_t REG_CTRL_MEAS = 0xF4;
static constexpr uint8_t REG_CONFIG    = 0xF5;
static constexpr uint8_t REG_PRESS_MSB = 0xF7; // F7..F9 pressure, FA..FC temp
static constexpr uint8_t REG_CALIB00   = 0x88; // 0x88..0xA1

static constexpr uint8_t CHIP_ID_BMP280 = 0x58;

bool PresBmp280::begin(TwoWire& wire, uint8_t addr7) {
  _wire = &wire;
  _addr = addr7;
  _ok = false;

  uint8_t id = 0;
  if (!read8(REG_ID, id)) return false;
  if (id != CHIP_ID_BMP280) return false;

  // Optional soft reset (kept off by default to avoid surprises)
  // write8(REG_RESET, 0xB6); delay(5);

  if (!read_calibration()) return false;
  if (!configure()) return false;

  _ok = true;
  return true;
}

void PresBmp280::read(PresSample& out) {
  out = PresSample{};
  if (!_ok) return;

  int32_t adc_T = 0, adc_P = 0;
  if (!read_raw(adc_T, adc_P)) {
    out.valid = false;
    return;
  }

  // Temperature: use Bosch compensation, output float °C
  int32_t t_x100 = compensate_T_x100(adc_T);
  float temp_c = (float)t_x100 / 100.0f;

  // Pressure: Bosch returns Q24.8 Pa; convert to Pa float
  uint32_t p_q24_8 = compensate_P_Q24_8(adc_P);
  float press_pa = (float)p_q24_8 / 256.0f;

  if (isnan(temp_c) || isnan(press_pa) || press_pa <= 0.0f) {
    out.valid = false;
    return;
  }

  out.valid = true;
  out.temp_c = temp_c;
  out.press_pa = press_pa;
}

bool PresBmp280::read8(uint8_t reg, uint8_t& v) {
  if (!_wire) return false;
  _wire->beginTransmission(_addr);
  _wire->write(reg);
  if (_wire->endTransmission(false) != 0) return false;
  if (_wire->requestFrom((int)_addr, 1) != 1) return false;
  v = _wire->read();
  return true;
}

bool PresBmp280::readN(uint8_t reg, uint8_t* buf, size_t n) {
  if (!_wire) return false;
  _wire->beginTransmission(_addr);
  _wire->write(reg);
  if (_wire->endTransmission(false) != 0) return false;
  if (_wire->requestFrom((int)_addr, (int)n) != (int)n) return false;
  for (size_t i = 0; i < n; i++) buf[i] = _wire->read();
  return true;
}

bool PresBmp280::write8(uint8_t reg, uint8_t v) {
  if (!_wire) return false;
  _wire->beginTransmission(_addr);
  _wire->write(reg);
  _wire->write(v);
  return (_wire->endTransmission(true) == 0);
}

bool PresBmp280::read_calibration() {
  uint8_t b[24] = {0};
  if (!readN(REG_CALIB00, b, sizeof(b))) return false;

  dig_T1 = u16_le(&b[0]);
  dig_T2 = i16_le(&b[2]);
  dig_T3 = i16_le(&b[4]);

  dig_P1 = u16_le(&b[6]);
  dig_P2 = i16_le(&b[8]);
  dig_P3 = i16_le(&b[10]);
  dig_P4 = i16_le(&b[12]);
  dig_P5 = i16_le(&b[14]);
  dig_P6 = i16_le(&b[16]);
  dig_P7 = i16_le(&b[18]);
  dig_P8 = i16_le(&b[20]);
  dig_P9 = i16_le(&b[22]);

  // Sanity: dig_P1 must be non-zero per datasheet
  if (dig_P1 == 0) return false;

  return true;
}

bool PresBmp280::configure() {
  // Phase-0 stable defaults:
  // - temp oversampling x2
  // - pressure oversampling x16
  // - normal mode
  // - IIR filter x16
  // - standby 62.5 ms (for normal mode)

  // config: t_sb(62.5ms)=0b001, filter(x16)=0b100, spi3w_en=0
  // 0xF5: [7:5]=t_sb, [4:2]=filter, [0]=spi3w_en
  uint8_t config = (0b001 << 5) | (0b100 << 2);

  // ctrl_meas: osrs_t(x2)=0b010, osrs_p(x16)=0b101, mode(normal)=0b11
  // 0xF4: [7:5]=osrs_t, [4:2]=osrs_p, [1:0]=mode
  uint8_t ctrl_meas = (0b010 << 5) | (0b101 << 2) | 0b11;

  if (!write8(REG_CONFIG, config)) return false;
  delay(2);
  if (!write8(REG_CTRL_MEAS, ctrl_meas)) return false;
  delay(2);

  return true;
}

bool PresBmp280::read_raw(int32_t& adc_T, int32_t& adc_P) {
  uint8_t b[6] = {0};
  if (!readN(REG_PRESS_MSB, b, sizeof(b))) return false;

  // Pressure: 20-bit unsigned
  adc_P = ((int32_t)b[0] << 12) | ((int32_t)b[1] << 4) | ((int32_t)b[2] >> 4);
  // Temp: 20-bit unsigned
  adc_T = ((int32_t)b[3] << 12) | ((int32_t)b[4] << 4) | ((int32_t)b[5] >> 4);

  return true;
}

// Bosch compensation (BMP280 datasheet reference implementation)
// Temperature in 0.01°C
int32_t PresBmp280::compensate_T_x100(int32_t adc_T) {
  int32_t var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * (int32_t)dig_T2) >> 11;
  int32_t var2 = (((((adc_T >> 4) - (int32_t)dig_T1) * ((adc_T >> 4) - (int32_t)dig_T1)) >> 12) *
                  (int32_t)dig_T3) >> 14;

  _t_fine = var1 + var2;
  int32_t T = (_t_fine * 5 + 128) >> 8; // 0.01°C
  return T;
}

// Pressure in Pa (Q24.8 format per Bosch algo)
uint32_t PresBmp280::compensate_P_Q24_8(int32_t adc_P) {
  int64_t var1 = (int64_t)_t_fine - 128000;
  int64_t var2 = var1 * var1 * (int64_t)dig_P6;
  var2 = var2 + ((var1 * (int64_t)dig_P5) << 17);
  var2 = var2 + (((int64_t)dig_P4) << 35);
  var1 = ((var1 * var1 * (int64_t)dig_P3) >> 8) + ((var1 * (int64_t)dig_P2) << 12);
  var1 = (((((int64_t)1) << 47) + var1) * (int64_t)dig_P1) >> 33;

  if (var1 == 0) return 0; // avoid div by zero

  int64_t p = 1048576 - adc_P;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 = ((int64_t)dig_P9 * (p >> 13) * (p >> 13)) >> 25;
  var2 = ((int64_t)dig_P8 * p) >> 19;

  p = ((p + var1 + var2) >> 8) + (((int64_t)dig_P7) << 4);

  // p is Q24.8 (Pa)
  if (p < 0) return 0;
  if (p > 0xFFFFFFFFLL) return 0xFFFFFFFFu;
  return (uint32_t)p;
}
