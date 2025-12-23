#include "state_machine.h"

void StateMachine::reset() {
  out_ = {};
  out_.state = FlightState::IDLE;
  out_.mode = FlightMode::IDLE;
  out_.armed = false;
  out_.kill_active = false;
}

StateMachineOutput StateMachine::update(const StateMachineInput& in) {
  // TODO: implement guarded transitions + failsafes
  if (in.cmd_kill) {
    out_.kill_active = true;
    out_.armed = false;
    out_.state = FlightState::KILLED;
    out_.mode = FlightMode::IDLE;
    return out_;
  }
  return out_;
}
