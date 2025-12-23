#pragma once

// Aggregated sensor interfaces (optional convenience header)

struct SensorHealth {
  bool ok = false;
  uint32_t last_update_us = 0;
};
