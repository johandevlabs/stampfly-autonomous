#pragma once
#include <Arduino.h>
#include <SPI.h>

extern "C" {
  #include "bmi2.h"
  #include "bmi270.h"

  // ---- ADD THESE DECLARATIONS ----
  int8_t bmi2_spi_read(uint8_t reg_addr,
                       uint8_t *reg_data,
                       uint32_t len,
                       void *intf_ptr);

  int8_t bmi2_spi_write(uint8_t reg_addr,
                        const uint8_t *reg_data,
                        uint32_t len,
                        void *intf_ptr);

  void bmi2_delay_us(uint32_t period, void *intf_ptr);
}

struct Bmi270SpiIntf {
  uint8_t cs_pin;
  SPISettings settings;
};
