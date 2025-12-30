#include "sensors/sensors.h"

bool Sensors::begin(TwoWire& wire) {
  bool ok = true;

  Serial.println("[sensors] begin");

  // IMU
  bool imu_ok = _imu.begin();
  Serial.printf("[sensors][imu]   BMI270: %s\n", imu_ok ? "OK" : "FAIL");
  if (!imu_ok) ok = false;

  // Flow
  bool flow_ok = _flow.begin();
  Serial.printf("[sensors][flow]  PMW3901: %s\n", flow_ok ? "OK" : "FAIL");
  if (!flow_ok) ok = false;

  // ToF (down)
  bool tof_down_ok = _tof_down.begin(_downPins, TOF_ADDR8_DOWN, TOF_ADDR7_DOWN);
  if (tof_down_ok) {
    _tof_down.start_ranging();
  }
  Serial.printf("[sensors][tof]   down: %s\n", tof_down_ok ? "OK" : "FAIL");
  if (!tof_down_ok) ok = false;

  // ToF (front) – currently optional
  /*
  bool tof_front_ok = _tof_front.begin(_frontPins, TOF_ADDR8_FRONT, TOF_ADDR7_FRONT);
  if (tof_front_ok) {
    _tof_front.start_ranging();
  }
  Serial.printf("[sensors][tof]   front: %s\n", tof_front_ok ? "OK" : "FAIL");
  if (!tof_front_ok) ok = false;
  */

  // Power monitor
  bool power_ok = _power.begin(wire, 0x40, 0.01f);
  Serial.printf("[sensors][power] INA3221: %s\n", power_ok ? "OK" : "FAIL");
  if (!power_ok) ok = false;

  // Pressure (BMP280)
  bool pres_ok = _pres.begin(wire, 0x76);
  Serial.printf("[sensors][pres]  BMP280: %s\n", pres_ok ? "OK" : "FAIL");
  if (!pres_ok) ok = false;

  // Magnetometer (BMM150)
  bool mag_ok = _mag.begin(wire, 0x10);
  Serial.printf("[sensors][mag]   BMM150: %s\n", mag_ok ? "OK" : "FAIL");
  if (!mag_ok) ok = false;

  Serial.printf("[sensors] begin result: %s\n", ok ? "OK" : "FAIL");
  return ok;
}


void Sensors::fast_read() {
  _s.t_fast_ms = millis();

  // IMU
  ImuSample imu_s;
  _imu.readFRU(imu_s);
  _s.imu = imu_s;
  _s.imu_valid = imu_s.valid;

  // Flow
  FlowSample flow_s;
  _flow.read(flow_s);
  _s.flow = flow_s;
  _s.flow_valid = flow_s.valid;
}

void Sensors::slow_read() {
  _s.t_slow_ms = millis();

  // ToF down
  TofSample down;
  _tof_down.read(down);
  _s.tof_down = down;
  _s.tof_down_valid = down.valid;

  // ToF front
  //TofSample front;
  //_tof_front.read(front);
  //_s.tof_front = front;
  //_s.tof_front_valid = front.valid;
}

void Sensors::very_slow_read() {
  _s.t_very_slow_ms = millis();

  // Power
  PowerSample power_s = _power.read();
  _s.power = power_s;
  _s.power_valid = power_s.valid;
  _s.power_err = _power.errorCount();

  // Pressure
  PresSample pres_s;
  _pres.read(pres_s);
  _s.pres = pres_s;
  _s.pres_valid = pres_s.valid;

  // Magnetometer
  MagSample mag_s;
  _mag.read(mag_s);
  _s.mag = mag_s;
  _s.mag_valid = mag_s.valid;
}

void Sensors::printSample() const {
  // IMU
  if (_s.imu_valid) {
    Serial.printf("[imu SI] acc=%.3f %.3f %.3f  gyr=%.3f %.3f %.3f\n",
                  _s.imu.ax, _s.imu.ay, _s.imu.az,
                  _s.imu.gx, _s.imu.gy, _s.imu.gz);
  } else {
    Serial.println("[imu SI] --");
  }

  // Flow
  if (_s.flow_valid) {
    Serial.printf("[flow raw] dx= %.3f dy= %.3f motion= %u quality= %u\n",
                  _s.flow.dx, _s.flow.dy,
                  (unsigned)_s.flow.motion,
                  (unsigned)_s.flow.quality);
  } else {
    Serial.println("[flow raw] --");
  }

  // ToF
  auto print_one = [](const char *name, const TofSample &ts) {
    if (!ts.valid) {
      Serial.printf("%s=-- ", name);
      return;
    }
    if (ts.stale) {
      Serial.printf("%s=%u mm* ", name, ts.range_mm);
    } else {
      Serial.printf("%s=%u mm (st=%u) ", name, ts.range_mm, ts.range_status);
    }
  };

  Serial.print("[tof] ");
  print_one("down", _s.tof_down);
  Serial.println();

  // Power
  if (!_s.power_valid) {
    Serial.printf("[power] INVALID (err=%lu)\n", (unsigned long)_s.power_err);
  } else {
    Serial.printf("[power] VBAT_IN=%.3f V  I=%.3f A  P=%.3f W\n",
                  _s.power.vbat_in_v, _s.power.ishunt_a, _s.power.p_w);
  }

    // Pressure
  if (_s.pres_valid) {
    Serial.printf("[pres] T=%.2f C  P=%.1f Pa\n", _s.pres.temp_c, _s.pres.press_pa);
  } else {
    Serial.println("[pres] --");
  }

  // Magnetometer
  if (_s.mag_valid) {
    Serial.printf("[mag] raw x=%d y=%d z=%d (id=0x%02X)\n",
                  _s.mag.x, _s.mag.y, _s.mag.z, _s.mag.chip_id);
  } else {
    Serial.println("[mag] --");
  }

}
