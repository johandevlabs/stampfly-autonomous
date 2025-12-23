#pragma once
#include <stdint.h>

struct PidConfig {
  float kp = 0;
  float ki = 0;
  float kd = 0;
  float i_min = -0.0f;
  float i_max =  0.0f;
  float out_min = -0.0f;
  float out_max =  0.0f;
};

class Pid {
 public:
  void configure(const PidConfig& cfg);
  void reset();
  float update(float error, float dt_s);

  // Optional: derivative on measurement / filtering can be added later

 private:
  PidConfig cfg_{};
  float i_ = 0;
  float prev_err_ = 0;
  bool has_prev_ = false;
};
