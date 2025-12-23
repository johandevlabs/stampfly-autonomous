#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("StampFly firmware skeleton booting...");
}

void loop() {
  // TODO: scheduler dispatch:
  // - rate loop
  // - attitude loop
  // - altitude loop
  // - velocity loop
  // - state machine
  // - telemetry
  delay(10);
}
