#include "imu_bmi270.h"
#include "config/spi_config.h"
#include <Arduino.h>
#include <SPI.h>

#include "bmi270_bosch_glue.h"

static bmi2_dev g_dev;
static Bmi270SpiIntf g_intf;

// --- Helper functions --- //
static bool read_raw_sample(float &gx, float &gy, float &gz)
{
  bmi2_sens_data data = {0};
  int8_t rslt = bmi2_get_sensor_data(&data, &g_dev);
  if (rslt != BMI2_OK) return false;

  // convert raw -> rad/s using your current range
  constexpr float INV_32768 = 1.0f / 32768.0f;
  constexpr float DEG2RAD = 3.14159265358979323846f / 180.0f;
  constexpr float GYR_RANGE_DPS = 2000.0f;  // match your configured range

  gx = (data.gyr.x * INV_32768) * (GYR_RANGE_DPS * DEG2RAD);
  gy = (data.gyr.y * INV_32768) * (GYR_RANGE_DPS * DEG2RAD);
  gz = (data.gyr.z * INV_32768) * (GYR_RANGE_DPS * DEG2RAD);
  return true;
}

bool ImuBmi270::calibrate_gyro()
{
  // Simple gyro bias calibration: keep the drone still during this
  constexpr int N = 500;
  float bgx = 0, bgy = 0, bgz = 0;
  int n_ok = 0;

  for (int i = 0; i < N; i++) {
    float gx, gy, gz;
    if (read_raw_sample(gx, gy, gz)) {
      bgx += gx; bgy += gy; bgz += gz;
      n_ok++;
    }
    delay(4);
  }

  if (n_ok > 0) {
    _gyro_bias_x = bgx / n_ok;
    _gyro_bias_y = bgy / n_ok;
    _gyro_bias_z = bgz / n_ok;
    Serial.printf("[imu] gyro bias rad/s: %.6f %.6f %.6f (n=%d)\n",
                  _gyro_bias_x, _gyro_bias_y, _gyro_bias_z, n_ok);
    return true;
  } else {
    Serial.println("[imu] gyro bias calibration failed!");
    return false;
  }
}

static inline void mapSensorToFRU(ImuSample &s)
{
  const float sx  = s.ax,  sy  = s.ay,  sz  = s.az;
  const float sgx = s.gx,  sgy = s.gy,  sgz = s.gz;

  // Body FRU mapping you derived:
  // body_x = sensor_y, body_y = sensor_x, body_z = sensor_z
  s.ax = sy;
  s.ay = sx;
  s.az = sz;

  s.gx = sgy;
  s.gy = sgx;
  s.gz = sgz;
}

// --- Core functions --- //
bool ImuBmi270::begin()
{
  Serial.println("[imu] begin started!");
  pinMode(PIN_CS_BMI270, OUTPUT);
  digitalWrite(PIN_CS_BMI270, HIGH);

  delay(100);

  // SPI pins are already set in board_init(): board_spi_begin(44,43,14,-1)
  // (so we don't call SPI.begin() here unless you want to make it standalone)
  // 

  g_intf.cs_pin = PIN_CS_BMI270;
  g_intf.settings = SPI_BMI270_SETTINGS;

  memset(&g_dev, 0, sizeof(g_dev));
  g_dev.intf = BMI2_SPI_INTF;
  g_dev.read = bmi2_spi_read;
  g_dev.write = bmi2_spi_write;
  g_dev.delay_us = bmi2_delay_us;
  g_dev.intf_ptr = &g_intf;

  // REQUIRED for BMI270 over SPI
  g_dev.dummy_byte = 1;

  delay(200);

  int8_t rslt = bmi270_init(&g_dev);
  if (rslt != BMI2_OK) {
    Serial.printf("[imu] bmi270_init failed: %d\n", rslt);
    _ok = false;
    return false;
  }

  // Sanity: read chip-id via Bosch
  uint8_t id = 0;
  (void)bmi2_get_regs(BMI2_CHIP_ID_ADDR, &id, 1, &g_dev);
  Serial.printf("[imu] chip id = 0x%02X (expect 0x24)\n", id);

  // Configure accel + gyro
  bmi2_sens_config cfg[2];
  cfg[0].type = BMI2_ACCEL;
  cfg[1].type = BMI2_GYRO;

  rslt = bmi2_get_sensor_config(cfg, 2, &g_dev);
  if (rslt != BMI2_OK) {
    Serial.printf("[imu] get_sensor_config failed: %d\n", rslt);
    _ok = false;
    return false;
  }

  cfg[0].cfg.acc.odr = BMI2_ACC_ODR_400HZ;
  cfg[0].cfg.acc.range = BMI2_ACC_RANGE_4G;
  cfg[0].cfg.acc.bwp = BMI2_ACC_NORMAL_AVG4;
  cfg[0].cfg.acc.filter_perf = BMI2_PERF_OPT_MODE;

  cfg[1].cfg.gyr.odr = BMI2_GYR_ODR_400HZ;
  cfg[1].cfg.gyr.range = BMI2_GYR_RANGE_2000;
  cfg[1].cfg.gyr.bwp = BMI2_GYR_NORMAL_MODE;
  cfg[1].cfg.gyr.noise_perf = BMI2_PERF_OPT_MODE;
  cfg[1].cfg.gyr.filter_perf = BMI2_PERF_OPT_MODE;

  rslt = bmi2_set_sensor_config(cfg, 2, &g_dev);
  if (rslt != BMI2_OK) {
    Serial.printf("[imu] set_sensor_config failed: %d\n", rslt);
    _ok = false;
    return false;
  }

  // Enable accel + gyro (Bosch API uses a sensor list)
  uint8_t sens_list[2] = { BMI2_ACCEL, BMI2_GYRO };
  rslt = bmi2_sensor_enable(sens_list, 2, &g_dev);
  if (rslt != BMI2_OK) {
    Serial.printf("[imu] sensor_enable failed: %d\n", rslt);
    _ok = false;
    return false;
  }

  if (rslt != BMI2_OK) {
    Serial.printf("[imu] sensor_enable failed: %d\n", rslt);
    _ok = false;
    return false;
  }

  delay(100);   // allow sensors to exit suspend and settle

  // Calibrate BMI270 gyro
  calibrate_gyro();
  
  _ok = true;
  Serial.println("[imu] BMI270 initialized (Bosch driver)");
  return true;
  
}

bool ImuBmi270::read(ImuSample &out)
{
  // --- scaling constants matching your config ---
  constexpr float G = 9.80665f;
  constexpr float ACC_RANGE_G = 4.0f;        // ±4g
  constexpr float GYR_RANGE_DPS = 2000.0f;   // ±2000 dps
  constexpr float INV_32768 = 1.0f / 32768.0f;
  constexpr float DEG2RAD = 3.14159265358979323846f / 180.0f;

  if (!_ok) {
    out.valid = false;
    return false;
  }

  bmi2_sens_data data = {0};
  int8_t rslt = bmi2_get_sensor_data(&data, &g_dev);
  if (rslt != BMI2_OK) {
    out.valid = false;
    return false;
  }

  out.ax = (data.acc.x * INV_32768) * (ACC_RANGE_G * G);
  out.ay = (data.acc.y * INV_32768) * (ACC_RANGE_G * G);
  out.az = (data.acc.z * INV_32768) * (ACC_RANGE_G * G);
  
  out.gx = (data.gyr.x * INV_32768) * (GYR_RANGE_DPS * DEG2RAD);
  out.gy = (data.gyr.y * INV_32768) * (GYR_RANGE_DPS * DEG2RAD);
  out.gz = (data.gyr.z * INV_32768) * (GYR_RANGE_DPS * DEG2RAD);

  out.gx -= _gyro_bias_x;
  out.gy -= _gyro_bias_y;
  out.gz -= _gyro_bias_z;

  out.t_us = micros();
  out.valid = true;
  return true;
}

// Returns IMU BMI270 readings in the Forward-Right-Up frame
bool ImuBmi270::readFRU(ImuSample &out)
{
  if (!read(out)) return false;
  mapSensorToFRU(out);
  return true;
}



