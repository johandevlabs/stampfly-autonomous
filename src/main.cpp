#include <Arduino.h>
#include <Wire.h>

#include "config/pins.h"
#include "utils/loop_stats.h"

#include "board/board_init.h"
#include "board/spi_probe.h"

#include "sensors/imu/imu_bmi270.h"
#include "sensors/flow/flow_pmw3901.h"
#include "sensors/tof/tof_vl53l3.h"

// Phase 0 loop targets
static constexpr uint32_t FAST_HZ = 250;   // e.g. IMU later
static constexpr uint32_t SLOW_HZ = 20;    // e.g. ToF later
static constexpr uint32_t REPORT_HZ = 1;

static constexpr uint32_t FAST_PERIOD_US   = 1000000UL / FAST_HZ;
static constexpr uint32_t SLOW_PERIOD_US   = 1000000UL / SLOW_HZ;
static constexpr uint32_t REPORT_PERIOD_US = 5000000UL / REPORT_HZ;

static LoopStats fast_stats;
static LoopStats slow_stats;
static uint32_t last_report_us = 0;

// Sensors
static ImuBmi270 g_imu;
static FlowPmw3901 g_flow;
static TofVl53L3 g_tof_down;
static TofVl53L3 g_tof_front;
TofPins downPins(PIN_TOF1_XSHUT, PIN_TOF1_GPIO1);
TofPins frontPins(PIN_TOF2_XSHUT, PIN_TOF2_GPIO1);


void setup() {
  Serial.begin(115200);
  delay(2500);
  Serial.println();
  Serial.println("StampFly Phase 0 bring-up starting...");

  // Init I2C and SPI then scan and print report
  auto init = board_init();
  //spi_probe_devices();

  // bring up IMU BMI270
  g_imu.begin();

  // bring up Flow PMW3901
  g_flow.begin();

  // bring up TOF VL53L3 sensors
  // ToFs are already held LOW by board_init().
  //digitalWrite(PIN_TOF2_XSHUT, LOW);  
  g_tof_down.begin(downPins, TOF_ADDR8_DOWN, TOF_ADDR7_DOWN);
  //g_tof_front.begin(frontPins, TOF_ADDR8_FRONT, TOF_ADDR7_FRONT);
  g_tof_down.start_ranging();
  //g_tof_front.start_ranging();
  
  auto scan = board_i2c_scan(Wire);
  scan.i2c_ok = init.i2c_ok;
  scan.spi_ok = init.spi_ok;
  board_print_report(scan);

  // Blink led
  /*pinMode(39, OUTPUT);
  digitalWrite(PIN_TOF1_XSHUT, LOW);
  delay(100);
  digitalWrite(PIN_TOF1_XSHUT, HIGH);
  delay(300);
  digitalWrite(PIN_TOF1_XSHUT, LOW);*/


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
    // TODO: IMU read
    static uint32_t last_imu_print_ms = 0;
    ImuSample s;
    
    if (g_imu.readFRU(s) && (millis() - last_imu_print_ms) >= 5000 ) {
      last_imu_print_ms = millis();
      Serial.printf("[imu SI] acc=%.3f %.3f %.3f  gyr=%.3f %.3f %.3f\n",
                    s.ax, s.ay, s.az, s.gx, s.gy, s.gz);
    }
  
    static uint32_t last_flow_print_ms = 0;
    FlowSample fs;
    if (g_flow.read(fs) && (millis() - last_flow_print_ms) >= 2000 ) {
      last_flow_print_ms = millis();
      Serial.printf("[flow raw] dx= %.3f dy= %.3f motion= %u quality= %u\n",
                    fs.dx, fs.dy, (unsigned)fs.motion, (unsigned)fs.quality);
    }
  }

  // Slow loop placeholder (later: ToF read)
  if (slow_stats.ready(now, SLOW_PERIOD_US)) {
    slow_stats.tick(now);

    TofSample down, front;
    g_tof_down.read(down);
    //g_tof_front.read(front);
    
    static uint32_t last_tof_print_ms = 0;
    if (millis() - last_tof_print_ms >= 2000) { // 2 Hz print to keep logs sane
      last_tof_print_ms = millis();

      auto print_one = [](const char *name, const TofSample &s) {
        if (!s.valid) {
          Serial.printf("%s=-- ", name);
          return;
        }
        if (s.stale) {
          Serial.printf("%s=%u mm* ", name, s.range_mm);
        } else {
          Serial.printf("%s=%u mm (st=%u) ", name, s.range_mm, s.range_status);
        }
      };

      Serial.print("[tof] ");
      print_one("down", down);
      //print_one("front", front);
      Serial.println();
    }
  }

  // 1 Hz report of dt jitter
  if ((uint32_t)(now - last_report_us) >= REPORT_PERIOD_US) {
    last_report_us = now;

    Serial.printf("[timing] fast(%u Hz): samples=%lu min=%luus avg=%luus max=%luus\n",
                  (unsigned)FAST_HZ,
                  (unsigned long)fast_stats.samples(),
                  (unsigned long)fast_stats.min_dt_us(),
                  (unsigned long)fast_stats.avg_dt_us(),
                  (unsigned long)fast_stats.max_dt_us());

    Serial.printf("[timing] slow(%u Hz): samples=%lu min=%luus avg=%luus max=%luus\n",
                  (unsigned)SLOW_HZ,
                  (unsigned long)slow_stats.samples(),
                  (unsigned long)slow_stats.min_dt_us(),
                  (unsigned long)slow_stats.avg_dt_us(),
                  (unsigned long)slow_stats.max_dt_us());
  }

  // Avoid starving Wi-Fi/RTOS housekeeping in future; safe to yield here.
  delay(1);
}
