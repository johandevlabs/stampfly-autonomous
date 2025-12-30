#include <Arduino.h>
#include <Wire.h>

#include "config/pins.h"
#include "utils/loop_stats.h"

#include "board/board_init.h"
#include "board/spi_probe.h"

#include "sensors/sensors.h"


// Phase 0 loop targets
static constexpr uint32_t FAST_HZ = 250;   // e.g. IMU later
static constexpr uint32_t SLOW_HZ = 20;    // e.g. ToF later
static constexpr uint32_t REPORT_HZ = 1;   // Report telemetry, and measure power

static constexpr uint32_t FAST_PERIOD_US   = 1000000UL / FAST_HZ;
static constexpr uint32_t SLOW_PERIOD_US   = 1000000UL / SLOW_HZ;
static constexpr uint32_t REPORT_PERIOD_US = 1000000UL / REPORT_HZ;

static LoopStats fast_stats;
static LoopStats slow_stats;
static uint32_t last_report_us = 0;

// Sensors
static Sensors g_sensors;

// temp helper functions - delete when done
static bool i2c_read_u8(uint8_t addr, uint8_t reg, uint8_t &value)
{
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  if (Wire.requestFrom(addr, (uint8_t)1) != 1) {
    return false;
  }

  value = Wire.read();
  return true;
}

void probe_i2c_devices()
{
  uint8_t id;

  Serial.println("=== I2C chip ID probe ===");

  // ---- BMP280 / BME280 @ 0x76 ----
  if (i2c_read_u8(0x76, 0xD0, id)) {
    Serial.printf("[0x76] chip ID = 0x%02X", id);
    if (id == 0x58) {
      Serial.println(" -> BMP280");
    } else if (id == 0x60) {
      Serial.println(" -> BME280");
    } else {
      Serial.println(" -> unknown (but alive)");
    }
  } else {
    Serial.println("[0x76] no response");
  }

  // ---- BMM150 @ 0x10 ----
  if (i2c_read_u8(0x10, 0x40, id)) {
    Serial.printf("[0x10] chip ID = 0x%02X", id);
    if (id == 0x32) {
      Serial.println(" -> BMM150");
    } else {
      Serial.println(" -> unexpected ID");
    }
  } else {
    Serial.println("[0x10] no response");
  }

  Serial.println("=========================");
}

static bool i2c_read_u8_rs(uint8_t addr, uint8_t reg, uint8_t &value)
{
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false; // repeated-start
  if (Wire.requestFrom(addr, (uint8_t)1) != 1) return false;
  value = Wire.read();
  return true;
}

static bool i2c_read_u8_stop(uint8_t addr, uint8_t reg, uint8_t &value)
{
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(true) != 0) return false; // stop
  if (Wire.requestFrom(addr, (uint8_t)1) != 1) return false;
  value = Wire.read();
  return true;
}

static void dump_reg(uint8_t addr, uint8_t reg)
{
  uint8_t v1=0xAA, v2=0xAA;
  bool ok1 = i2c_read_u8_rs(addr, reg, v1);
  bool ok2 = i2c_read_u8_stop(addr, reg, v2);

  Serial.printf("[0x%02X] reg 0x%02X  RS:%s 0x%02X  STOP:%s 0x%02X\n",
                addr, reg,
                ok1 ? "OK " : "FAIL", v1,
                ok2 ? "OK " : "FAIL", v2);
}

static bool i2c_write_u8(uint8_t addr, uint8_t reg, uint8_t value)
{
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(value);
  return (Wire.endTransmission(true) == 0);
}

void probe_bmm150_like(uint8_t addr)
{
  Serial.println("=== BMM150-ish probe @0x10 ===");

  // Dump some candidate ID / status regs
  dump_reg(addr, 0x40); // BMM150 chip-id (expected 0x32)
  dump_reg(addr, 0x41); // often used as a follow-up sanity check on BMM150
  dump_reg(addr, 0x4B); // power control on BMM150
  dump_reg(addr, 0x4C); // opmode on BMM150

  // Try "power on" (BMM150: set bit0 in 0x4B)
  Serial.println("Trying power control: write 0x01 to reg 0x4B ...");
  bool w = i2c_write_u8(addr, 0x4B, 0x01);
  Serial.printf("write: %s\n", w ? "OK" : "FAIL");
  delay(10);

  // Re-read chip id after power-up attempt
  dump_reg(addr, 0x40);

  Serial.println("=============================");
}




void setup() {
  Serial.begin(115200);
  delay(2500);
  Serial.println("StampFly Phase 0 bring-up starting...");

  // Init I2C and SPI then scan and print report
  auto init = board_init();
  //spi_probe_devices();

  // Bring up all sensors (Wire is already initialized in board_init())
  bool sensors_ok = g_sensors.begin(Wire);

  // Run a final i2c scan and print results  
  auto scan = board_i2c_scan(Wire);
  scan.i2c_ok = init.i2c_ok;
  scan.spi_ok = init.spi_ok;
  board_print_report(scan);

  // temp - delete when done
  delay(100);
  probe_bmm150_like(0x10);
  delay(100);
  probe_i2c_devices();

  // Init timing stats
  fast_stats.reset();
  slow_stats.reset();
  last_report_us = micros();
}


void loop() {
  const uint32_t now = micros();

  // Fast loop placeholder (later: IMU read)
  if (fast_stats.ready(now, FAST_PERIOD_US)) {
    fast_stats.tick(now);

    // Read 'fast' sensors    
    g_sensors.fast_read();

  }

  // Slow loop placeholder (later: ToF read)
  if (slow_stats.ready(now, SLOW_PERIOD_US)) {
    slow_stats.tick(now);

    // Read 'slow' sensors
    g_sensors.slow_read();

  }

  // 1 Hz report of dt jitter
  if ((uint32_t)(now - last_report_us) >= REPORT_PERIOD_US) {
    last_report_us = now;

    // Update very slow sensors (power)
    g_sensors.very_slow_read();
    g_sensors.printSample();

    // Print timing stats
    Serial.printf("[timing] fast(%u Hz): samples=%lu min=%luus avg=%luus max=%luus\n",
                  (unsigned)FAST_HZ, (unsigned long)fast_stats.samples(),
                  (unsigned long)fast_stats.min_dt_us(), (unsigned long)fast_stats.avg_dt_us(),
                  (unsigned long)fast_stats.max_dt_us());
    
    Serial.printf("[timing] slow(%u Hz): samples=%lu min=%luus avg=%luus max=%luus\n",
                  (unsigned)SLOW_HZ, (unsigned long)slow_stats.samples(),
                  (unsigned long)slow_stats.min_dt_us(), (unsigned long)slow_stats.avg_dt_us(),
                  (unsigned long)slow_stats.max_dt_us());
  }
  
  // Avoid starving Wi-Fi/RTOS housekeeping in future; safe to yield here.
  //delay(1);
  //delayMicroseconds(10);
}
