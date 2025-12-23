# Command Protocol

This document defines the **command and telemetry protocol** used between the
StampFly drone and external controllers (ESP-NOW, UDP, or future transports).

It is intentionally:
- Simple
- Explicit
- Versioned
- Transport-agnostic

This file is a **capsule context** reference and should be kept in sync with
`src/comms/packet_defs.h`.

---

## Design Goals

1. Drone owns **stability and safety**
2. User sends **intent**, not motor commands
3. Same packet format for all transports
4. Easy to inspect and debug
5. Graceful handling of packet loss

---

## Transport Independence

The protocol is **payload-only**.
Transport layers (ESP-NOW, UDP, BLE) must not:
- Add semantics
- Modify meaning
- Reorder fields

All intelligence lives on the drone.

---

## Packet Overview

Two primary packet directions:

- **Command packets** → controller → drone
- **Telemetry packets** → drone → controller

All packets:
- Fixed-size structs
- Little-endian
- No dynamic fields

---

## Packet Versioning

Every packet begins with:

| Field | Type | Description |
|----|----|-------------|
| `version` | `uint8_t` | Protocol version |
| `type` | `uint8_t` | Packet type enum |
| `seq` | `uint16_t` | Sequence counter |

Version mismatches must cause the packet to be ignored.

---

## Command Packet

### CommandPacket (controller → drone)

```
struct CommandPacket {
  uint8_t  version;
  uint8_t  type;
  uint16_t seq;

  uint8_t  mode;
  uint8_t  flags;

  float    z_setpoint_cm;
  float    vx_setpoint_cm_s;
  float    vy_setpoint_cm_s;

  float    dx_cm;
  float    dy_cm;
  float    dz_cm;
};
```

### Field Semantics

| Field | Meaning |
|----|---------|
| `mode` | Requested flight mode |
| `flags` | Bitfield (ARM, KILL, etc.) |
| `z_setpoint_cm` | Target altitude |
| `vx_setpoint_cm_s` | X velocity |
| `vy_setpoint_cm_s` | Y velocity |
| `dx_cm` | Delta X (motion primitive) |
| `dy_cm` | Delta Y |
| `dz_cm` | Delta Z |

Unused fields must be set to zero.

---

## Flight Modes

```
enum FlightMode {
  MODE_IDLE = 0,
  MODE_ALT_HOLD,
  MODE_VEL_HOLD,
  MODE_LAND,
  MODE_FAILSAFE
};
```

Rules:
- Mode changes are **requests**, not guarantees
- Drone may reject invalid transitions
- FAILSAFE overrides all other modes

---

## Command Flags

```
bit 0: ARM
bit 1: DISARM
bit 2: KILL
bit 3: TAKEOFF
bit 4: LAND
```

Rules:
- `KILL` always wins
- `ARM` requires IDLE state
- `DISARM` only allowed when landed

---

## Motion Primitives

### MOVE_DELTA

To request a relative move:
- Set mode = MODE_VEL_HOLD
- Populate `dx_cm`, `dy_cm`, `dz_cm`
- Set velocities to zero

The drone:
- Converts deltas into timed velocity profiles
- Enforces limits
- Executes autonomously

---

## Telemetry Packet

### TelemetryPacket (drone → controller)

```
struct TelemetryPacket {
  uint8_t  version;
  uint8_t  type;
  uint16_t seq;

  uint8_t  mode;
  uint8_t  state;

  float    roll_deg;
  float    pitch_deg;
  float    yaw_deg;

  float    z_cm;
  float    vx_cm_s;
  float    vy_cm_s;

  float    battery_v;
  uint32_t flags;
};
```

---

## Telemetry Flags

Examples:
- SENSOR_OK
- LINK_OK
- BATTERY_LOW
- FAILSAFE_ACTIVE

Flags are bitfields and cumulative.

---

## Timing & Heartbeat

- Command packets should arrive at ≥10 Hz
- If no valid command for >300 ms → HOLD
- If no valid command for >1–2 s → LAND
- Telemetry rate:
  - Fast: 20–50 Hz
  - Slow: 1–5 Hz

---

## Error Handling

- Invalid packet → ignore
- Version mismatch → ignore
- Bad mode transition → reject
- Link loss → controlled LAND
- Sensor fault → FAILSAFE

---

## Debugging Recommendations

- Log packet sequence numbers
- Expose rejected-command counters
- Mirror last valid command in telemetry
- Always test with packet loss simulation

---

## Design Rules (Do Not Break)

1. Never expose motor control
2. Never trust the controller
3. Drone is always authoritative
4. Safety beats obedience
5. Defaults must be safe

---

## Final Notes

This protocol is intentionally conservative.
You can *add* fields later, but removing ambiguity early
will save you months of debugging.
