# Safety Model

This document defines the **safety philosophy, mechanisms, and failure handling**
for the StampFly semi-autonomous drone.

Safety is treated as a **first-class system**, not a side effect of control logic.

This file is a **capsule context** document and must be kept consistent with:
- `state_machine.*`
- `failsafe.*`
- `command_protocol.md`

---

## Safety Philosophy

1. **The drone is always authoritative**
2. **Loss of information must degrade behavior, not amplify it**
3. **Unsafe commands are rejected, not “best-effort”**
4. **When in doubt, land**
5. **A stopped drone is safer than a clever one**

Safety logic must be:
- Explicit
- Centralized
- Predictable

---

## Safety Layers

Safety is enforced at multiple layers:

```
User Commands
      ↓
Command Validation
      ↓
State Machine Guards
      ↓
Control Limits
      ↓
Failsafe Overrides
      ↓
Motor Output
```

A violation at *any* layer must prevent unsafe behavior.

---

## Arming & Disarming Rules

### ARM
Allowed only if:
- State == IDLE
- No active failsafe
- Battery voltage above minimum
- Sensors healthy

### DISARM
Allowed only if:
- State == IDLE or LANDED
- Throttle is zero

### KILL
- Immediate motor shutdown
- No checks
- Overrides all states and modes

---

## Flight States & Safety Expectations

| State | Allowed Behavior |
|----|------------------|
| IDLE | Motors off |
| ARMING | No motor spin |
| TAKEOFF | Limited throttle & tilt |
| ALT_HOLD | Constrained angles |
| VEL_HOLD | Constrained velocity |
| LANDING | No climb allowed |
| FAILSAFE | Autonomous landing |
| KILLED | Motors forced off |

---

## Command Validation

Before accepting a command:
- Packet version must match
- Mode transition must be legal
- Setpoints must be within limits
- Flags must be coherent

Invalid commands:
- Are ignored
- Increment a rejection counter
- Are reported via telemetry

---

## Link-Loss Handling

### Timeouts
- >300 ms without valid command → HOLD
- >1–2 s without valid command → LAND
- Persistent loss → DISARM after landing

### Rules
- Last valid setpoint may be held briefly
- No new motion is initiated without link
- KILL is always local and immediate

---

## Sensor Health Monitoring

Each sensor reports:
- Initialization status
- Update freshness
- Sanity checks (range, NaN)

### Sensor Failure Response

| Sensor | Failure Action |
|------|----------------|
| IMU | Immediate FAILSAFE |
| ToF | Disable altitude hold |
| Optical flow | Disable velocity hold |
| Barometer | Ignore |
| Battery monitor | Conservative LAND |

---

## Battery Safety

### Thresholds
- **Warning:** telemetry only
- **Low:** initiate LAND
- **Critical:** forced DISARM after landing

Battery state must never:
- Increase throttle
- Delay landing

---

## Control Limits

Hard limits enforced regardless of mode:

- Max tilt angle
- Max angular rate
- Max throttle
- Max climb rate
- Max horizontal velocity
- Max altitude

Limits are:
- Centralized
- Compile-time defaults
- Telemetry-visible

---

## Failsafe Mode

Triggered by:
- IMU failure
- Persistent link loss
- Internal consistency errors
- Manual FAILSAFE request

Behavior:
- Switch to FAILSAFE state
- Attempt controlled landing
- Ignore external commands
- Allow KILL only

---

## Ground Effect & Low-Altitude Risks

Special handling below ~10 cm:
- Reduced throttle authority
- Increased damping
- No horizontal motion initiation

Prevents “ground cushion runaway”.

---

## Logging & Observability

Safety-relevant events must:
- Increment counters
- Emit telemetry flags
- Be debuggable post-flight

Never silently fail.

---

## Design Rules (Non-Negotiable)

1. Safety code lives in `safety/`
2. Control loops never bypass safety
3. Safety checks never allocate memory
4. Safety behavior must be deterministic
5. A KILL command always works

---

## Common Failure Scenarios

| Scenario | Expected Outcome |
|--------|------------------|
| Wi-Fi drop | HOLD → LAND |
| Bad packet | Ignored |
| IMU glitch | FAILSAFE |
| Low battery | LAND |
| CPU overload | FAILSAFE |
| User error | Rejected command |

---

## Final Notes

Safety logic should feel **boring and conservative**.

If you are ever tempted to “just let it try”,
that logic belongs in a simulator — not on flying hardware.
