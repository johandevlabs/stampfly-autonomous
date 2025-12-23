#pragma once
#include <stdint.h>
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

// Phase 0: bring-up helpers.
// NOTE: StampFly uses a specific pinout; if Wire.begin() doesn't find devices,
// set pins in config/pins.h and call board_i2c_begin_with_pins().

struct BoardBringupReport {
  bool i2c_ok = false;
  bool spi_ok = false;
  uint8_t i2c_devices_found = 0;
  uint8_t i2c_addrs[32] = {0};
};

// Initialize buses and basic peripherals needed for Phase 0.
BoardBringupReport board_init();

// Call Wire.begin() with explicit SDA/SCL pins (if needed).
bool board_i2c_begin_with_pins(int sda, int scl, uint32_t freq_hz = 400000);

// I2C scan utility (Wire must be started).
BoardBringupReport board_i2c_scan(TwoWire& bus = Wire, uint8_t addr_min = 0x08, uint8_t addr_max = 0x77);

// SPI init helper (safe no-op wrapper).
bool board_spi_begin(int sck = -1, int miso = -1, int mosi = -1, int ss = -1);

// Print a bring-up report to Serial.
void board_print_report(const BoardBringupReport& r);
