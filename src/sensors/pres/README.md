# pres_bmp280

Bring-up driver for BMP280 barometric pressure sensor.

## Address
- StampFly: 0x76 (7-bit)

## What it does
- Initializes BMP280 using Adafruit_BMP280
- Reads:
  - temperature (°C)
  - pressure (Pa)

## Notes
- Phase 0: no altitude calc, no filtering beyond BMP internal config.
