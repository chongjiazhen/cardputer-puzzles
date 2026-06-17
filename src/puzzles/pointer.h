#pragma once
namespace puz {

struct Pointer { float x, y; };

inline float applyDeadzone(float a, float dz) {
  if (a > dz)  return a - dz;
  if (a < -dz) return a + dz;
  return 0.0f;
}

inline void pointerStep(Pointer& p, float ax, float ay, float dt, int w, int h) {
  const float DZ = 0.08f, SPEED = 220.0f;
  p.x += applyDeadzone(ax, DZ) * SPEED * dt;
  p.y += applyDeadzone(ay, DZ) * SPEED * dt;
  if (p.x < 0) p.x = 0; if (p.x > w - 1) p.x = float(w - 1);
  if (p.y < 0) p.y = 0; if (p.y > h - 1) p.y = float(h - 1);
}

}  // namespace puz
