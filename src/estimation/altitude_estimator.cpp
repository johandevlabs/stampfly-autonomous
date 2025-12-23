#include "altitude_estimator.h"

bool AltitudeEstimator::begin() {
  s_ = {};
  return true;
}

void AltitudeEstimator::update_tof(float range_cm, float dt_s) {
  // TODO: fuse ToF into altitude state
  (void)range_cm; (void)dt_s;
  s_.valid = false;
}

void AltitudeEstimator::update_baro(float pressure_pa, float dt_s) {
  // TODO: optional baro fusion
  (void)pressure_pa; (void)dt_s;
}

AltitudeState AltitudeEstimator::state() const { return s_; }
