# pres_bmp280

Phase-0 bring-up driver for the **Bosch BMP280** pressure sensor.

This implementation is intentionally **register-level and explicit**:
- No Adafruit wrapper
- No hidden `Wire.begin()` calls
- Uses Bosch datasheet compensation to output temperature + pressure

## Hardware / Address
- StampFly BMP280: **0x76** (7-bit I2C)

## What it provides
Cached sample fields (see `PresSample`):
- `temp_c` — temperature in °C
- `press_pa` — pressure in Pa
- `valid` — true if a full read + compensation succeeded

## Driver behavior (Phase 0)
### `begin()`
- Reads chip ID at `0xD0` and requires **0x58**
- Reads calibration block from `0x88..0x9F`
- Configures the sensor for stable continuous sampling:
  - Temp oversampling: x2
  - Pressure oversampling: x16
  - IIR filter: x16
  - Standby: 62.5 ms
  - Mode: normal

### `read()`
- Reads raw pressure + temperature (`0xF7..0xFC`)
- Applies Bosch reference compensation
- Returns values in engineering units (°C, Pa)

## Non-goals (by design)
- No altitude calculation here (handled later in estimators)
- No advanced filtering beyond BMP280 internal IIR filter
- No fusion with ToF / velocity estimators

## Debug tips
- If pressure reads **0 Pa** or `valid=false`:
  - verify I2C scan sees `0x76`
  - verify chip ID reads `0x58`
  - check pullups / bus power integrity
