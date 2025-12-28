#pragma once
#include <Arduino.h>

struct ImuSample {
  float ax, ay, az;  // m/s^2 (or g if you prefer; we’ll document once confirmed)
  float gx, gy, gz;  // rad/s (or dps)
  uint32_t t_us;
  bool valid;
};

class ImuBmi270 {
public:
  bool begin();
  bool readFRU(ImuSample &out); // returns in FRU frame

private:
  //static bool _read_raw_sample(float &x, float &y, float &z);
  bool read(ImuSample &out); // returns in BMI270s default frame
  bool calibrate_gyro();
  bool _ok = false;
  float _gyro_bias_x =0;
  float _gyro_bias_y =0;
  float _gyro_bias_z =0;
};
