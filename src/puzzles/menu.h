#pragma once
#include "frontend.h"

// gamelist[] and gamecount are declared by puzzles.h (included via frontend.h)

namespace puz {
inline void drawMenu(int sel) {
  auto &d = M5.Display;
  d.fillScreen(TFT_BLACK);
  d.setTextSize(1);
  d.setTextDatum(top_left);
  for (int i = 0; i < gamecount; i++) {
    d.setTextColor(i == sel ? TFT_BLACK : TFT_WHITE,
                   i == sel ? TFT_WHITE : TFT_BLACK);
    d.drawString(gamelist[i]->name, 6, 4 + i * 14);
  }
}
}  // namespace puz
