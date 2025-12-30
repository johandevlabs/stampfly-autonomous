#include "sensors/pres/pres_bmp280.h"
#include <Adafruit_BMP280.h>

static Adafruit_BMP280 g_bmp;

bool PresBmp280::begin(TwoWire& wire, uint8_t addr7) {
  (void)wire; // this Adafruit_BMP280 variant uses global Wire

  _ok = g_bmp.begin(addr7);
  if (_ok) {
    g_bmp.setSampling(
      Adafruit_BMP280::MODE_NORMAL,
      Adafruit_BMP280::SAMPLING_X2,
      Adafruit_BMP280::SAMPLING_X16,
      Adafruit_BMP280::FILTER_X16,
      Adafruit_BMP280::STANDBY_MS_63
    );
  }
  return _ok;
}

void PresBmp280::read(PresSample& out) {
  out = PresSample{};
  if (!_ok) return;

  float t = g_bmp.readTemperature(); // °C
  float p = g_bmp.readPressure();    // Pa

  if (isnan(t) || isnan(p) || p <= 0.0f) {
    out.valid = false;
    return;
  }

  out.valid = true;
  out.temp_c = t;
  out.press_pa = p;
}
