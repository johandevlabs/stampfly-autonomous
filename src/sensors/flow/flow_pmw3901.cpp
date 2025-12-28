#include "flow_pmw3901.h"

#include <Arduino.h>
#include <SPI.h>

// Your project-specific SPI config lives here (as you described):
//  - SPI_PMW3901_SETTINGS(1000000, MSBFIRST, SPI_MODE0)
//  - PIN_CS_PMW3901 = 12
#include "config/spi_config.h"

namespace {

// PMW3901 typically wants a dummy byte on reads.
constexpr uint8_t DUMMY = 0x00;

// Conservative CS timing guards (ESP32-S3 is fast; the sensor can be picky)
inline void cs_select() {
  digitalWrite(PIN_CS_PMW3901, LOW);
  delayMicroseconds(5);
}
inline void cs_deselect() {
  delayMicroseconds(5);
  digitalWrite(PIN_CS_PMW3901, HIGH);
  delayMicroseconds(5);
}

// NOTE: Addressing differs across PMW3901 breakouts/drivers.
// Many use: write = reg | 0x80, read = reg & 0x7F.
// If your first probe returns nonsense, we’ll flip this convention.
static uint8_t reg_read(uint8_t reg) {
  // adapted from BitCraze PMW3901 lib
  reg &= ~0x80u;
  SPI.beginTransaction(SPI_PMW3901_SETTINGS);
  digitalWrite(PIN_CS_PMW3901, LOW);
  
  delayMicroseconds(50);
  SPI.transfer(reg);
  delayMicroseconds(50);          // tSRAD: allow sensor to prepare data
  uint8_t v = SPI.transfer(0x00); // clock out data
  delayMicroseconds(100);

  digitalWrite(PIN_CS_PMW3901, HIGH);
  SPI.endTransaction();

  delayMicroseconds(50);          // inter-transaction gap (safe/conservative)
  return v;
}

inline void reg_write(uint8_t reg, uint8_t val) {
  // Adapted from Bitcraze pmw3901 lib
  reg |= 0x80u;
  SPI.beginTransaction(SPI_PMW3901_SETTINGS);
  cs_select();

  delayMicroseconds(50);
  SPI.transfer(reg);
  SPI.transfer(val);
  delayMicroseconds(50);

  cs_deselect();
  SPI.endTransaction();
  delayMicroseconds(200);
}

}  // namespace

// Performance optimisation registers
// Taken from BitCraze PMW3901 lib
void initRegisters()
{
  reg_write(0x7F, 0x00);
  reg_write(0x61, 0xAD);
  reg_write(0x7F, 0x03);
  reg_write(0x40, 0x00);
  reg_write(0x7F, 0x05);
  reg_write(0x41, 0xB3);
  reg_write(0x43, 0xF1);
  reg_write(0x45, 0x14);
  reg_write(0x5B, 0x32);
  reg_write(0x5F, 0x34);
  reg_write(0x7B, 0x08);
  reg_write(0x7F, 0x06);
  reg_write(0x44, 0x1B);
  reg_write(0x40, 0xBF);
  reg_write(0x4E, 0x3F);
  reg_write(0x7F, 0x08);
  reg_write(0x65, 0x20);
  reg_write(0x6A, 0x18);
  reg_write(0x7F, 0x09);
  reg_write(0x4F, 0xAF);
  reg_write(0x5F, 0x40);
  reg_write(0x48, 0x80);
  reg_write(0x49, 0x80);
  reg_write(0x57, 0x77);
  reg_write(0x60, 0x78);
  reg_write(0x61, 0x78);
  reg_write(0x62, 0x08);
  reg_write(0x63, 0x50);
  reg_write(0x7F, 0x0A);
  reg_write(0x45, 0x60);
  reg_write(0x7F, 0x00);
  reg_write(0x4D, 0x11);
  reg_write(0x55, 0x80);
  reg_write(0x74, 0x1F);
  reg_write(0x75, 0x1F);
  reg_write(0x4A, 0x78);
  reg_write(0x4B, 0x78);
  reg_write(0x44, 0x08);
  reg_write(0x45, 0x50);
  reg_write(0x64, 0xFF);
  reg_write(0x65, 0x1F);
  reg_write(0x7F, 0x14);
  reg_write(0x65, 0x60);
  reg_write(0x66, 0x08);
  reg_write(0x63, 0x78);
  reg_write(0x7F, 0x15);
  reg_write(0x48, 0x58);
  reg_write(0x7F, 0x07);
  reg_write(0x41, 0x0D);
  reg_write(0x43, 0x14);
  reg_write(0x4B, 0x0E);
  reg_write(0x45, 0x0F);
  reg_write(0x44, 0x42);
  reg_write(0x4C, 0x80);
  reg_write(0x7F, 0x10);
  reg_write(0x5B, 0x02);
  reg_write(0x7F, 0x07);
  reg_write(0x40, 0x41);
  reg_write(0x70, 0x00);

  delay(100);
  reg_write(0x32, 0x44);
  reg_write(0x7F, 0x07);
  reg_write(0x40, 0x40);
  reg_write(0x7F, 0x06);
  reg_write(0x62, 0xf0);
  reg_write(0x63, 0x00);
  reg_write(0x7F, 0x0D);
  reg_write(0x48, 0xC0);
  reg_write(0x6F, 0xd5);
  reg_write(0x7F, 0x00);
  reg_write(0x5B, 0xa0);
  reg_write(0x4E, 0xA8);
  reg_write(0x5A, 0x50);
  reg_write(0x40, 0x80);
}


bool FlowPmw3901::begin() {
  pinMode(PIN_CS_PMW3901, OUTPUT);
  digitalWrite(PIN_CS_PMW3901, HIGH);  // idle HIGH (board_init already does it, but harmless)
  SPI.beginTransaction(SPI_PMW3901_SETTINGS);

  // Make sure the SPI bus is reset
  digitalWrite(PIN_CS_PMW3901, HIGH);
  delay(1);
  digitalWrite(PIN_CS_PMW3901, LOW);
  delay(1);
  digitalWrite(PIN_CS_PMW3901, HIGH);
  delay(1);

  SPI.endTransaction();

  // Power on reset
  reg_write(0x3A, 0x5A);
  delay(5);
  // Test the SPI communication, checking chipId and inverse chipId
  uint8_t chipId = reg_read(0x00);
  uint8_t dIpihc = reg_read(0x5F);

  if (chipId != 0x49 || dIpihc != 0xB6) {
    return false;
  }

  // Reading the motion registers one time
  reg_read(0x02);
  reg_read(0x03);
  reg_read(0x04);
  reg_read(0x05);
  reg_read(0x06);
  delay(1);

  initRegisters();

  return true;
}

bool FlowPmw3901::read(FlowSample& out) {
  // Timestamp early so the caller knows "when this was checked"
  out.t_us = micros();

  // Always read motion + quality (useful even when no new dx/dy sample exists)
  const uint8_t m = reg_read(0x02);
  const uint8_t q = reg_read(0x07);

  out.motion = m;
  out.quality = q;
  out.quality_ok = (q >= 30);

  // MOTION GATE: bit7 indicates a new motion sample is available
  const bool has_new_motion = (m & 0x80) != 0;
  if (!has_new_motion) {
    out.dx = 0.0f;
    out.dy = 0.0f;
    out.valid = false;
    return false;                 // <-- key behavior change
  }

  // Only read dx/dy when there's fresh motion data
  const int16_t dx = (int16_t)((uint16_t(reg_read(0x04)) << 8) | reg_read(0x03));
  const int16_t dy = (int16_t)((uint16_t(reg_read(0x06)) << 8) | reg_read(0x05));

  out.dx = (float)dx;
  out.dy = (float)dy;
  out.valid = true;
  return true;
}
