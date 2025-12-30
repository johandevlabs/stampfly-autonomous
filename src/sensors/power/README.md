# Power Monitor (INA3221) – Phase 0.5 bring-up

This module reads the onboard INA3221 current/voltage monitor to observe **VBAT_IN rail health**
and correlate sensor failures / brownouts with **battery sag**.

## I2C address
Scan shows `0x40` present, which matches INA3221 default (A0=GND).  
Other detected devices: `0x10, 0x30, 0x76` (not handled here).

## Wiring on StampFly
Only INA3221 "Channel 2" is used:

- IN2+ / IN2- measure across a **0.01 Ω shunt**
- Shunt feeds the rail **VBAT_IN** (post-MOSFET, post-shunt)
- VBAT path:
  VBAT -> AP50P20 (P-MOSFET) -> IN2+ -> (0.01Ω) -> IN2- -> VBAT_IN -> loads -> GND
- VPU is tied to VBAT_IN

Important implications:
- **Bus voltage (CH2)** = approximately **VBAT_IN** (what the electronics/motors see)
- **Current (CH2)** = shunt current (I = Vshunt / 0.01Ω), computed by library after setShuntR()
- Raw battery voltage **before the MOSFET** is NOT measured directly.

## Library channel mapping
RobTillaart INA3221 library uses 0-based channel indexing: 0,1,2.
StampFly wiring uses INA "CH2" -> library channel index **1**.

## Update rate
- For bring-up, printing at 5–10 Hz is useful to observe transients.
- For normal operation, 1 Hz reporting is sufficient and reduces I2C traffic.

## Failure mode note
Because VPU is tied to VBAT_IN, a strong droop can cause the INA3221 to vanish from I2C.
Treat repeated `isConnected()==false` or read failures as a strong brownout indicator.
