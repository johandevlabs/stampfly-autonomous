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
  delay(1);
}
