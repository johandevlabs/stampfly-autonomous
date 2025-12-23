#include "spi_probe.h"
#include "config/spi_config.h"
#include <SPI.h>

static uint8_t spi_read_raw(int cs, uint8_t reg, const SPISettings& s)
{
  uint8_t v;
  SPI.beginTransaction(s);
  digitalWrite(cs, LOW);
  SPI.transfer(reg);
  v = SPI.transfer(0x00);
  digitalWrite(cs, HIGH);
  SPI.endTransaction();
  return v;
}

void spi_probe_devices()
{
  pinMode(PIN_CS_BMI270, OUTPUT);
  pinMode(PIN_CS_PMW3901, OUTPUT);
  digitalWrite(PIN_CS_BMI270, HIGH);
  digitalWrite(PIN_CS_PMW3901, HIGH);
  delay(10);

  Serial.println("\n[SPI PROBE] locked configuration");

  uint8_t bmi_id = spi_read_raw(PIN_CS_BMI270, 0x00, SPI_BMI270_SETTINGS);
  Serial.printf("BMI270 probe (MODE0 @1MHz): 0x%02X\n", bmi_id);

  uint8_t pmw_id = spi_read_raw(PIN_CS_PMW3901, 0x00, SPI_PMW3901_SETTINGS);
  Serial.printf("PMW3901 probe (MODE0 @1MHz): 0x%02X\n", pmw_id);
}
