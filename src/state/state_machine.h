#pragma once
#include <stdint.h>
#include "flight_mode.h"

enum class FlightState : uint8_t {
  IDLE = 0,
  ARMING,
  TAKEOFF,
  CONTROLLED,
  LANDING,
  FAILSAFE,
  KILLED
};

struct StateMachineInput {
  FlightMode requested_mode = FlightMode::IDLE;
  bool cmd_arm = false;
  bool cmd_disarm = false;
  bool cmd_kill = false;
  bool cmd_land = false;
  bool link_ok = false;
  bool sensors_ok = false;
  bool battery_low = false;
  bool on_ground = true;
};

struct StateMachineOutput {
  FlightState state = FlightState::IDLE;
  FlightMode mode = FlightMode::IDLE;
  bool armed = false;
  bool kill_active = false;
};

class StateMachine {
 public:
  void reset();
  StateMachineOutput update(const StateMachineInput& in);

 private:
  StateMachineOutput out_{};
};
