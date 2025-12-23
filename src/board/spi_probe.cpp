#include "spi_probe.h"
#include <SPI.h>

static constexpr int PIN_CS_BMI270  = 46;
static constexpr int PIN_CS_PMW3901 = 12;

// Simple SPI register read helpers (7-bit addr; MSB often indicates R/W on many devices).
// We keep it generic and just try common conventions.

static uint8_t spi_read_reg_common(int cs_pin, uint8_t reg, const SPISettings& settings,
                                  bool read_bit_msb, bool auto_inc)
{
  // Two common conventions:
  // 1) read_bit_msb: set bit7=1 for read, bit7=0 for write
  // 2) auto_inc: set bit6=1 for multi-byte read
  uint8_t addr = reg;
  if (read_bit_msb) addr |= 0x80;
  if (auto_inc)     addr |= 0x40;

  uint8_t val = 0x00;

  SPI.beginTransaction(settings);
  digitalWrite(cs_pin, LOW);
  SPI.transfer(addr);
  val = SPI.transfer(0x00);
  digitalWrite(cs_pin, HIGH);
  SPI.endTransaction();

  return val;
}

static void print_try_matrix(const char* name, int cs_pin, uint8_t reg)
{
  pinMode(cs_pin, OUTPUT);
  digitalWrite(cs_pin, HIGH);
  delay(2);

  // Try reasonable speeds. Start slow.
  const SPISettings s_mode0_1mhz(1000000, MSBFIRST, SPI_MODE0);
  const SPISettings s_mode3_1mhz(1000000, MSBFIRST, SPI_MODE3);
  const SPISettings s_mode0_4mhz(4000000, MSBFIRST, SPI_MODE0);
  const SPISettings s_mode3_4mhz(4000000, MSBFIRST, SPI_MODE3);

  struct Try { const char* tag; SPISettings s; bool msb; bool inc; };
  Try tries[] = {
    {"M0 1MHz Rbit", s_mode0_1mhz, true,  false},
    {"M0 1MHz raw ", s_mode0_1mhz, false, false},
    {"M3 1MHz Rbit", s_mode3_1mhz, true,  false},
    {"M3 1MHz raw ", s_mode3_1mhz, false, false},

    {"M0 4MHz Rbit", s_mode0_4mhz, true,  false},
    {"M0 4MHz raw ", s_mode0_4mhz, false, false},
    {"M3 4MHz Rbit", s_mode3_4mhz, true,  false},
    {"M3 4MHz raw ", s_mode3_4mhz, false, false},
  };

  Serial.printf("\n[SPI PROBE] %s (CS=%d) reg 0x%02X\n", name, cs_pin, reg);
  for (auto &t : tries) {
    uint8_t v = spi_read_reg_common(cs_pin, reg, t.s, t.msb, t.inc);
    Serial.printf("  %-10s -> 0x%02X\n", t.tag, v);
    delay(2);
  }
}

static void dump_regs(const char* name,
                      int cs_pin,
                      uint8_t start_reg,
                      uint8_t count,
                      const SPISettings& settings,
                      uint32_t hz,
                      uint8_t mode)
{
  Serial.printf("\n[SPI DUMP] %s (CS=%d) regs 0x%02X..0x%02X (mode=%u, %lu Hz)\n",
                name, cs_pin, start_reg, (uint8_t)(start_reg + count - 1),
                (unsigned)mode, (unsigned long)hz);

  pinMode(cs_pin, OUTPUT);
  digitalWrite(cs_pin, HIGH);
  delay(2);

  SPI.beginTransaction(settings);
  digitalWrite(cs_pin, LOW);

  // Read-bit-in-MSB + auto-inc attempt
  uint8_t addr = start_reg | 0x80 | 0x40;
  SPI.transfer(addr);

  for (uint8_t i = 0; i < count; i++) {
    uint8_t v = SPI.transfer(0x00);
    Serial.printf("  0x%02X: 0x%02X\n", (uint8_t)(start_reg + i), v);
  }

  digitalWrite(cs_pin, HIGH);
  SPI.endTransaction();
}

void spi_probe_devices()
{
  // Quick “matrix” read of likely ID registers:
  // BMI270 chip-id register is commonly at 0x00 on many Bosch IMUs (we won't assume value).
  // PMW3901 also has ID-ish registers near 0x00/0x01 in many implementations.
  print_try_matrix("BMI270?",  PIN_CS_BMI270,  0x00);
  print_try_matrix("PMW3901?", PIN_CS_PMW3901, 0x00);

  // If you see non-0x00/0xFF values in one mode consistently, do a small dump.
  // Dump using Mode0 at 1MHz as a safe default.
  const uint32_t HZ1 = 1000000;
  const uint32_t HZ4 = 4000000;

  const SPISettings safe0_1(HZ1, MSBFIRST, SPI_MODE0);
  const SPISettings safe3_1(HZ1, MSBFIRST, SPI_MODE3);

  dump_regs("BMI270? (M0)",  PIN_CS_BMI270,  0x00, 8, safe0_1, HZ1, 0);
  dump_regs("BMI270? (M3)",  PIN_CS_BMI270,  0x00, 8, safe3_1, HZ1, 3);

  dump_regs("PMW3901? (M0)", PIN_CS_PMW3901, 0x00, 8, safe0_1, HZ1, 0);
  dump_regs("PMW3901? (M3)", PIN_CS_PMW3901, 0x00, 8, safe3_1, HZ1, 3);

}
