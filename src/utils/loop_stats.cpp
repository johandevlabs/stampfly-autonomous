#include "loop_stats.h"

void LoopStats::reset() {
  last_us_ = 0;
  min_dt_us_ = 0xFFFFFFFF;
  max_dt_us_ = 0;
  sum_dt_us_ = 0;
  samples_ = 0;
}

bool LoopStats::ready(uint32_t now_us, uint32_t period_us) const {
  if (last_us_ == 0) return true;
  return (uint32_t)(now_us - last_us_) >= period_us;
}

void LoopStats::tick(uint32_t now_us) {
  if (last_us_ != 0) {
    uint32_t dt = (uint32_t)(now_us - last_us_);
    if (dt < min_dt_us_) min_dt_us_ = dt;
    if (dt > max_dt_us_) max_dt_us_ = dt;
    sum_dt_us_ += dt;
    samples_++;
  }
  last_us_ = now_us;
}
