#pragma once
#include <stdint.h>

// Simple loop dt statistics (microseconds).
// Use for Phase 0 to validate scheduler timing and jitter.
class LoopStats {
 public:
  void reset();
  void tick(uint32_t now_us);
  bool ready(uint32_t now_us, uint32_t period_us) const;

  uint32_t last_us() const { return last_us_; }
  uint32_t min_dt_us() const { return min_dt_us_; }
  uint32_t max_dt_us() const { return max_dt_us_; }
  uint32_t avg_dt_us() const { return samples_ ? (sum_dt_us_ / samples_) : 0; }
  uint32_t samples() const { return samples_; }

 private:
  uint32_t last_us_ = 0;
  uint32_t min_dt_us_ = 0xFFFFFFFF;
  uint32_t max_dt_us_ = 0;
  uint64_t sum_dt_us_ = 0;
  uint32_t samples_ = 0;
};
