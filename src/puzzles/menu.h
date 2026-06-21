#pragma once
#include "picker.h"
#include "frontend.h"   // gamelist[]/gamecount (puzzles.h) + M5.Display

namespace puz {

// Display names cached once (gamelist order is the alphabetical menu order).
inline const char** chooserNames() {
  static const char* names[64];
  static bool init = false;
  if (!init) { for (int i = 0; i < gamecount; i++) names[i] = gamelist[i]->name; init = true; }
  return names;
}

inline void drawChooser(int sel) {
  static char pos[12];
  snprintf(pos, sizeof pos, "%d/%d", sel + 1, gamecount);
  drawPicker(chooserNames(), gamecount, sel, "Puzzles", pos, nullptr);
  auto &d = M5.Display;                       // footer hint
  d.setTextSize(1); d.setTextColor(d.color565(0x67, 0x88, 0x99), TFT_BLACK);
  d.setTextDatum(bottom_left); d.drawString("Enter: play  Tab: help", 4, 133);
}

}  // namespace puz
