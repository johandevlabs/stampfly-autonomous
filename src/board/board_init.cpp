#include "board_init.h"
#include "config/spi_config.h"
#include "config/pins.h"

BoardBringupReport board_init() {
  BoardBringupReport r;

  r.i2c_ok = board_i2c_begin_with_pins(PIN_I2C_SDA, PIN_I2C_SCL, I2C_FREQ);
  r.spi_ok = board_spi_begin(44, 43, 14, -1);

  // Make sure all SPI devices are deselected BEFORE SPI starts
  pinMode(PIN_CS_BMI270, OUTPUT);
  digitalWrite(PIN_CS_BMI270, HIGH);

  pinMode(PIN_CS_PMW3901, OUTPUT);
  digitalWrite(PIN_CS_PMW3901, HIGH);
  
  // (Add any other SPI CS pins here)

  // TOF-sensors: Hold both sensors in reset at boot (Phase 0 deterministic)
  pinMode(PIN_TOF1_XSHUT, OUTPUT);
  pinMode(PIN_TOF2_XSHUT, OUTPUT);
  digitalWrite(PIN_TOF1_XSHUT, LOW);
  digitalWrite(PIN_TOF2_XSHUT, LOW);
  delay(20);

  // Optional: data-ready pins as inputs
  pinMode(PIN_TOF1_GPIO1, INPUT);
  pinMode(PIN_TOF2_GPIO1, INPUT);

  return r;
}


bool board_i2c_begin_with_pins(int sda, int scl, uint32_t freq_hz) {
  if (sda < 0 || scl < 0) return false;
  bool ok = Wire.begin(sda, scl);
  if (ok) Wire.setClock(freq_hz);
  return ok;
}

BoardBringupReport board_i2c_scan(TwoWire& bus, uint8_t addr_min, uint8_t addr_max) {
  BoardBringupReport r;
  r.i2c_ok = true;

  uint8_t found = 0;
  for (uint8_t addr = addr_min; addr <= addr_max; addr++) {
    bus.beginTransmission(addr);
    uint8_t err = bus.endTransmission();
    if (err == 0) {
      if (found < 32) {
        r.i2c_addrs[found] = addr;
      }
      found++;
      delay(2);
    }
  }
  r.i2c_devices_found = found;
  return r;
}

bool board_spi_begin(int sck, int miso, int mosi, int ss) {
  // If pins are left as -1, use default SPI pins for the selected board.
  // For StampFly you may need to provide explicit pins later.
  if (sck >= 0 && miso >= 0 && mosi >= 0) {
    SPI.begin(sck, miso, mosi, ss);
  } else {
    SPI.begin();
  }
  return true;
}

void board_print_report(const BoardBringupReport& r) {
  Serial.println();
  Serial.println("=== Phase 0 Bring-up Report ===");
  Serial.printf("I2C ok: %s\n", r.i2c_ok ? "yes" : "no");
  Serial.printf("SPI ok: %s\n", r.spi_ok ? "yes" : "no");
  Serial.printf("I2C devices found: %u\n", r.i2c_devices_found);

  if (r.i2c_devices_found > 0) {
    Serial.print("I2C addresses: ");
    uint8_t n = r.i2c_devices_found > 32 ? 32 : r.i2c_devices_found;
    for (uint8_t i = 0; i < n; i++) {
      Serial.printf("0x%02X", r.i2c_addrs[i]);
      if (i + 1 < n) Serial.print(", ");
    }
    if (r.i2c_devices_found > 32) Serial.print(" ...");
    Serial.println();
  }
  Serial.println("================================");
  Serial.println();
}
