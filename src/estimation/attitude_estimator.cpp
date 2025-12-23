#include "attitude_estimator.h"

bool AttitudeEstimator::begin() {
  s_ = {};
  return true;
}

void AttitudeEstimator::update(float gx_dps, float gy_dps, float gz_dps,
                               float ax_g, float ay_g, float az_g,
                               float dt_s) {
  // TODO: implement complementary / Mahony / Madgwick filter
  (void)gx_dps; (void)gy_dps; (void)gz_dps;
  (void)ax_g; (void)ay_g; (void)az_g;
  (void)dt_s;
  s_.valid = false;
}

AttitudeState AttitudeEstimator::state() const { return s_; }
