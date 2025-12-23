#include "pid.h"

static float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

void Pid::configure(const PidConfig& cfg) { cfg_ = cfg; }
void Pid::reset() { i_ = 0; prev_err_ = 0; has_prev_ = false; }

float Pid::update(float error, float dt_s) {
  if (dt_s <= 0) return 0;

  // P
  float p = cfg_.kp * error;

  // I with clamp
  i_ += cfg_.ki * error * dt_s;
  i_ = clampf(i_, cfg_.i_min, cfg_.i_max);

  // D
  float d = 0;
  if (has_prev_) {
    d = cfg_.kd * (error - prev_err_) / dt_s;
  }
  prev_err_ = error;
  has_prev_ = true;

  float out = p + i_ + d;
  out = clampf(out, cfg_.out_min, cfg_.out_max);
  return out;
}
