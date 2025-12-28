#include "bmi270_bosch_glue.h"

extern "C" {

// Bosch uses this callback
void bmi2_delay_us(uint32_t period, void * /*intf_ptr*/)
{
  delayMicroseconds(period);
}

// IMPORTANT DUMMY BYTE NOTE:
// Bosch driver already accounts for the dummy byte by calling read() with
// len = requested_len + dev->dummy_byte.
// Therefore: DO NOT strip/shift bytes here.
int8_t bmi2_spi_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
  auto *ctx = reinterpret_cast<Bmi270SpiIntf *>(intf_ptr);
  if (!ctx || !reg_data || len == 0) return BMI2_E_NULL_PTR;

  const uint8_t addr = reg_addr | 0x80; // read bit

  SPI.beginTransaction(ctx->settings);
  digitalWrite(ctx->cs_pin, LOW);

  SPI.transfer(addr);
  for (uint32_t i = 0; i < len; i++) {
    reg_data[i] = SPI.transfer(0x00);
  }

  digitalWrite(ctx->cs_pin, HIGH);
  SPI.endTransaction();
  return BMI2_OK;
}

int8_t bmi2_spi_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
  auto *ctx = reinterpret_cast<Bmi270SpiIntf *>(intf_ptr);
  if (!ctx || (!reg_data && len > 0)) return BMI2_E_NULL_PTR;

  const uint8_t addr = reg_addr & 0x7F; // write

  SPI.beginTransaction(ctx->settings);
  digitalWrite(ctx->cs_pin, LOW);

  SPI.transfer(addr);
  for (uint32_t i = 0; i < len; i++) {
    SPI.transfer(reg_data[i]);
  }

  digitalWrite(ctx->cs_pin, HIGH);
  SPI.endTransaction();
  return BMI2_OK;
}

} // extern "C"
