#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <stdint.h>
#include <vl53lx_class.h>

struct TofPins {
  int xshut = -1;
  int gpio1 = -1;

  constexpr TofPins() = default;
  constexpr TofPins(int x, int g) : xshut(x), gpio1(g) {}
};

struct TofSample {
  uint32_t t_us = 0;

  bool valid = false;
  bool stale = false;

  uint16_t range_mm = 0;
  uint8_t range_status = 0;

  uint16_t ambient = 0;
  uint16_t signal = 0;
  uint8_t  stream_count = 0;
};


class TofVl53L3 {
public:
  ~TofVl53L3();

  bool begin(TofPins pins, uint8_t addr8_init, uint8_t addr7_expected);
  void end();               // optional
  bool present() const;
  uint8_t addr7() const { return addr7_; }
  bool start_ranging();              // call once after both sensors are up
  bool read(TofSample& out);         // non-blocking: valid=false if nothing new

private:
  static bool probe_(TwoWire& w, uint8_t addr7);

  TwoWire* wire_ = &Wire;
  TofPins pins_;
  uint8_t addr7_ = 0x00;
  uint8_t addr8_ = 0x00;
  bool ranging_started_ = false;
  uint32_t last_fresh_us_ = 0; // last good reading (used to decide if reset of sensor is necces.)

  VL53LX* dev_ = nullptr;

  struct LastGood_ {
    bool has = false;
    uint32_t t_us = 0;
    uint16_t range_mm = 0;
    uint8_t  range_status = 0;
    uint16_t ambient = 0;
    uint16_t signal = 0;
    uint8_t  stream_count = 0;
  } last_good_;

};
