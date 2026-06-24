#pragma once
#include <cstdint>

namespace puz {

// Idle-based backlight level (pure, host-testable).
//   idleMs  — ms since last input
//   awake   — the user's normal brightness
// Bands: [0, dimMs) -> awake, [dimMs, offMs) -> dimLevel, [offMs, inf) -> 0 (off).
inline uint8_t idleBrightness(uint32_t idleMs, uint8_t awake,
                              uint32_t dimMs, uint32_t offMs, uint8_t dimLevel) {
  if (idleMs >= offMs) return 0;
  if (idleMs >= dimMs) return dimLevel;
  return awake;
}

}  // namespace puz
