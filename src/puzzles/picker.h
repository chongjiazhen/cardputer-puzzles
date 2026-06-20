#pragma once
#include "input.h"   // pulls <vector>; keeps this header Arduino-free for host tests

namespace puz {

struct PickerWindow { int idx[7]; int count; int selSlot; };

// Circular sel-centered window of up to maxRows (<=7) item indices.
// If n <= maxRows, shows all n with no wrap-duplication.
inline PickerWindow pickerWindow(int n, int sel, int maxRows) {
  PickerWindow w{};
  if (maxRows > 7) maxRows = 7;
  if (n <= maxRows) {
    for (int i = 0; i < n; i++) w.idx[i] = i;
    w.count = n; w.selSlot = sel;
    return w;
  }
  int half = maxRows / 2;
  for (int s = 0; s < maxRows; s++) {
    int i = sel - half + s;
    w.idx[s] = ((i % n) + n) % n;     // wrap into [0,n)
  }
  w.count = maxRows; w.selSlot = half;
  return w;
}

// Device render (defined in picker.cpp). suffix may be nullptr (no right column).
void drawPicker(const char* const* items, int n, int sel,
                const char* title, const char* pos,
                const char* const* suffix);

}  // namespace puz
