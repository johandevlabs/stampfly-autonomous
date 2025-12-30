# mag_bmm150

Phase-0 bring-up driver for BMM150 magnetometer.

## Address
- StampFly: 0x10 (7-bit)

## Key gotcha
BMM150 may boot in a state where CHIP_ID reads 0x00 until power control is enabled:
- write 0x01 to reg 0x4B
- then reg 0x40 should read 0x32

## What it does
- Enables power control (0x4B = 0x01)
- Verifies CHIP_ID (0x40 == 0x32)
- Reads raw XYZ from 0x42..0x47

## Notes
- Raw readings are not calibrated (hard/soft iron, offsets).
- Phase 0 goal is stable readout + wiring confirmation.
