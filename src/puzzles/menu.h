#pragma once
#include "frontend.h"

// gamelist[] and gamecount are declared by puzzles.h (included via frontend.h)

namespace puz {
inline void drawMenu(int sel) {
  auto &d = M5.Display;
  d.fillScreen(TFT_BLACK);
  d.setTextSize(1);
  d.setTextDatum(top_left);
  const int rowH = 14, top = 4;
  const int rows = (d.height() - top) / rowH;   // items that fit on screen
  int first = sel - rows / 2;                    // scroll window, sel centered
  if (first > gamecount - rows) first = gamecount - rows;
  if (first < 0) first = 0;
  for (int r = 0; r < rows && first + r < gamecount; r++) {
    int i = first + r;
    d.setTextColor(i == sel ? TFT_BLACK : TFT_WHITE,
                   i == sel ? TFT_WHITE : TFT_BLACK);
    d.drawString(gamelist[i]->name, 6, top + r * rowH);
  }
}
}  // namespace puz
