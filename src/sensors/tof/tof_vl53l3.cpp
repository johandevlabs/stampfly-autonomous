#include "tof_vl53l3.h"
#include <cstring>

bool TofVl53L3::probe_(TwoWire& w, uint8_t addr7) {
  w.beginTransmission(addr7);
  return (w.endTransmission() == 0);
}

TofVl53L3::~TofVl53L3() {
  end();
}

void TofVl53L3::end() {
  if (dev_) {
    delete dev_;
    dev_ = nullptr;
  }

  if (pins_.xshut >= 0) {
    pinMode(pins_.xshut, OUTPUT);
    digitalWrite(pins_.xshut, LOW); // hold in reset
  }
}

bool TofVl53L3::begin(TofPins pins, uint8_t addr8_init, uint8_t addr7_expected) {
  pins_ = pins;
  addr8_ = addr8_init;
  addr7_ = addr7_expected;

  // Release from reset
  pinMode(pins_.xshut, OUTPUT);
  digitalWrite(pins_.xshut, HIGH);
  delay(50);

  // Recreate device on heap (avoid stack temporaries)
  if (dev_) {
    delete dev_;
    dev_ = nullptr;
  }
  dev_ = new VL53LX(wire_, pins_.xshut);
  if (!dev_) return false;

  // InitSensor uses 8-bit address in this stack
  // If your local InitSensor() returns void, remove the ret-check and rely on probe.
  int ret = dev_->InitSensor(addr8_);
  if (ret != 0) return false;

  delay(10);

  // Verify it responds on expected 7-bit address
  return probe_(*wire_, addr7_);
}

bool TofVl53L3::present() const {
  return probe_(*wire_, addr7_);
}

bool TofVl53L3::start_ranging()
{
  if (!dev_) return false;

  // This is the common pattern in ST VL53LX wrappers:
  // - clear any pending interrupt
  // - start measurement
  dev_->VL53LX_ClearInterruptAndStartMeasurement();
  ranging_started_ = true;
  last_fresh_us_ = micros();
  return true;
}

bool TofVl53L3::read(TofSample& out)
{
  // Explicit invalid defaults (so callers never see "random" fields)
  out = TofSample{};
  out.t_us = micros();

  static constexpr uint32_t HOLD_US = 300000; // 300 ms
  static constexpr uint16_t MIN_MM  = 20;
  static constexpr uint16_t MAX_MM  = 4000;

  static constexpr uint16_t INVALID_U16 = 0xFFFF;
  static constexpr uint8_t  INVALID_U8  = 0xFF;

  auto set_invalid = [&]() {
    out.valid = false;
    out.stale = false;

    out.range_mm = INVALID_U16;
    out.range_status = INVALID_U8;

    out.ambient = INVALID_U16;
    out.signal = INVALID_U16;
    out.stream_count = INVALID_U8;
  };

  auto use_last_good = [&]() -> bool {
    if (last_good_.has && (out.t_us - last_good_.t_us) < HOLD_US) {
      out.valid = true;
      out.stale = true;

      out.range_mm = last_good_.range_mm;
      out.range_status = INVALID_U8; // mark "stale" (not a fresh HW status)

      out.ambient = last_good_.ambient;
      out.signal = last_good_.signal;
      out.stream_count = last_good_.stream_count;
      return true;
    }

    set_invalid();
    return true;
  };

  if (!dev_ || !ranging_started_) {
    set_invalid();
    return false;
  }

  // Non-blocking ready check
  uint8_t ready = 0;
  int ret = dev_->VL53LX_GetMeasurementDataReady(&ready);
  static constexpr uint32_t STUCK_US = 400000; // 400 ms
  if (ready == 0) {
    // If we haven't seen a fresh frame for a while, re-kick
    const uint32_t now = out.t_us;
    if (last_fresh_us_ != 0 && (now - last_fresh_us_) > STUCK_US) {
      //dev_->VL53LX_StartMeasurement();
      last_fresh_us_ = now; // prevent rapid re-kicks
      //Serial.print("ready="); Serial.print(ready); Serial.print(" ret=");Serial.println(ret);
    }
    return use_last_good();
  }
  if (ret != 0) {
    return use_last_good();
  }

  // Read multi-ranging data
  VL53LX_MultiRangingData_t data;
  std::memset(&data, 0, sizeof(data));

  ret = dev_->VL53LX_GetMultiRangingData(&data);
  last_fresh_us_ = out.t_us;
  if (ret != 0) {
    return use_last_good();
  }

  // Populate from first object
  const auto& r0 = data.RangeData[0];

  out.range_mm     = r0.RangeMilliMeter;
  out.range_status = r0.RangeStatus;
  out.stream_count = data.StreamCount;

  out.ambient = r0.AmbientRateRtnMegaCps;
  out.signal  = r0.SignalRateRtnMegaCps;

  // Re-arm for next measurement ASAP
  dev_->VL53LX_ClearInterruptAndStartMeasurement();

  // Validity gating
  const bool sane_range =
      (out.range_mm != 8191) &&
      (out.range_mm >= MIN_MM) &&
      (out.range_mm <= MAX_MM);

  const bool ok_status = (out.range_status == 0);

  out.valid = ok_status && sane_range;
  out.stale = false;

  if (out.valid) {
    // Save full last-good packet
    last_good_.has = true;
    last_good_.t_us = out.t_us;

    last_good_.range_mm = out.range_mm;
    last_good_.range_status = out.range_status;
    last_good_.ambient = out.ambient;
    last_good_.signal = out.signal;
    last_good_.stream_count = out.stream_count;

    return true;
  }

  // Fresh sample was invalid -> hold last-good briefly (or invalidate)
  return use_last_good();
}
