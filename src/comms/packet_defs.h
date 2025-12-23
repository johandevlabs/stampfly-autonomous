#pragma once
#include <stdint.h>

// Keep in sync with docs/command_protocol.md

static constexpr uint8_t PROTOCOL_VERSION = 1;

enum class PacketType : uint8_t {
  COMMAND = 1,
  TELEMETRY = 2
};

enum CommandFlags : uint8_t {
  FLAG_ARM    = 1 << 0,
  FLAG_DISARM = 1 << 1,
  FLAG_KILL   = 1 << 2,
  FLAG_TAKEOFF= 1 << 3,
  FLAG_LAND   = 1 << 4
};

struct PacketHeader {
  uint8_t  version;
  uint8_t  type;
  uint16_t seq;
} __attribute__((packed));

struct CommandPacket {
  PacketHeader hdr;
  uint8_t mode;
  uint8_t flags;
  uint16_t _rsv;

  float z_setpoint_cm;
  float vx_setpoint_cm_s;
  float vy_setpoint_cm_s;

  float dx_cm;
  float dy_cm;
  float dz_cm;
} __attribute__((packed));

struct TelemetryPacket {
  PacketHeader hdr;
  uint8_t mode;
  uint8_t state;
  uint16_t _rsv;

  float roll_deg;
  float pitch_deg;
  float yaw_deg;

  float z_cm;
  float vx_cm_s;
  float vy_cm_s;

  float battery_v;
  uint32_t flags;
} __attribute__((packed));
