// pointer_test.cpp — g++ -std=c++17 pointer_test.cpp -o pointer_test && ./pointer_test
#include "pointer.h"
#include <cassert>
#include <cmath>
#include <cstdio>
using puz::Pointer; using puz::pointerStep;

int main() {
  // inside deadzone -> no movement
  { Pointer p{100, 60}; pointerStep(p, 0.05f, 0.0f, 0.1f, 240, 135);
    assert(p.x == 100 && p.y == 60); }
  // tilt right -> x increases by speed*dt (0.5g over deadzone)
  { Pointer p{100, 60}; pointerStep(p, 0.5f, 0.0f, 1.0f, 240, 135);
    assert(p.x > 100 && std::fabs(p.x - (100 + 220*(0.5f - 0.08f))) < 1.0f); }
  // clamp to right edge
  { Pointer p{239, 60}; pointerStep(p, 1.0f, 0.0f, 1.0f, 240, 135);
    assert(p.x == 239); }
  // clamp to left/top edge at 0
  { Pointer p{0, 0}; pointerStep(p, -1.0f, -1.0f, 1.0f, 240, 135);
    assert(p.x == 0 && p.y == 0); }
  printf("ok\n");
  return 0;
}
