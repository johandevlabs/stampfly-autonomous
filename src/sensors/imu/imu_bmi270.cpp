#include "imu_bmi270.h"
#include "config/spi_config.h"
#include <Arduino.h>
#include <SPI.h>

#include "bmi270_bosch_glue.h"

static bmi2_dev g_dev;
static Bmi270SpiIntf g_intf;

// --- Helper functions --- //
static bool read_raw_sample(float &ax, float &ay, float &az,
                            float &gx, float &gy, float &gz)
{
  bmi2_sens_data data = {0};
  int8_t rslt = bmi2_get_sensor_data(&data, &g_dev);
  if (rslt != BMI2_OK) return false;

  // Convert raw -> SI using your configured ranges
  constexpr float INV_32768 = 1.0f / 32768.0f;
  constexpr float DEG2RAD = 3.14159265358979323846f / 180.0f;
  constexpr float G = 9.80665f;

  constexpr float ACC_RANGE_G = 4.0f;        // match your config
  constexpr float GYR_RANGE_DPS = 2000.0f;   // match your config

  ax = (data.acc.x * INV_32768) * (ACC_RANGE_G * G);
  ay = (data.acc.y * INV_32768) * (ACC_RANGE_G * G);
  az = (data.acc.z * INV_32768) * (ACC_RANGE_G * G);

  gx = (data.gyr.x * INV_32768) * (GYR_RANGE_DPS * DEG2RAD);
  gy = (data.gyr.y * INV_32768) * (GYR_RANGE_DPS * DEG2RAD);
  gz = (data.gyr.z * INV_32768) * (GYR_RANGE_DPS * DEG2RAD);

  return true;
}

bool ImuBmi270::calibrate_gyro()
{
  // Keep drone still during this
  constexpr int N = 500;               // target samples (~2s at 250 Hz)
  constexpr int MIN_OK = 350;          // require enough "still" samples
  constexpr float G = 9.80665f;

  // Gating thresholds
  constexpr float GYRO_MAX_RAD_S = 0.15f;     // ~8.6 deg/s
  constexpr float ACC_MAG_TOL = 0.5f;         // m/s^2 tolerance around |g|

  float bgx = 0, bgy = 0, bgz = 0;
  int n_ok = 0;

  for (int i = 0; i < N; i++) {
    float ax, ay, az, gx, gy, gz;
    if (read_raw_sample(ax, ay, az, gx, gy, gz)) {

      const float gmag = sqrtf(gx*gx + gy*gy + gz*gz);
      const float amag = sqrtf(ax*ax + ay*ay + az*az);

      const bool still_gyro = (gmag < GYRO_MAX_RAD_S);
      const bool still_acc  = (fabsf(amag - G) < ACC_MAG_TOL);

      if (still_gyro && still_acc) {
        bgx += gx; bgy += gy; bgz += gz;
        n_ok++;
      }
    }
    delay(4); // ~250 Hz pacing
  }

  if (n_ok >= MIN_OK) {
    _gyro_bias_x = bgx / n_ok;
    _gyro_bias_y = bgy / n_ok;
    _gyro_bias_z = bgz / n_ok;

    Serial.printf("[imu] gyro bias rad/s: %.6f %.6f %.6f (n=%d/%d)\n",
                  _gyro_bias_x, _gyro_bias_y, _gyro_bias_z, n_ok, N);
    return true;
  }

  Serial.printf("[imu] gyro bias calibration rejected: n_ok=%d/%d (keep old bias)\n", n_ok, N);
  return false;
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

  int8_t rslt = bmi270_init(&g_dev);
  if (rslt != BMI2_OK) {
    Serial.printf("[imu] bmi270_init failed: %d\n", rslt);
    _ok = false;
    return false;
  }
  delay(10);

  // Sanity: read chip-id via Bosch
  uint8_t id = 0;
  (void)bmi2_get_regs(BMI2_CHIP_ID_ADDR, &id, 1, &g_dev);
  Serial.printf("[imu] chip id = 0x%02X (expect 0x24)\n", id);

  delay(10);

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

  delay(10);   // allow sensors to exit suspend and settle

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



