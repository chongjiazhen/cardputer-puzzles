#pragma once
#include "picker.h"
#include "frontend.h"   // gamelist[]/gamecount (puzzles.h) + M5.Display
#include "cardputer_hw.h"   // cardputer::battery()

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
  drawPicker(chooserNames(), gamecount, sel, "Puzzles", pos, nullptr, 5);  // 5 rows: roomy after header+footer
  auto &d = M5.Display;                       // footer hint
  d.setTextSize(1); d.setTextColor(d.color565(0x67, 0x88, 0x99), TFT_BLACK);
  d.setTextDatum(bottom_left); d.drawString("Enter: play  Tab: help", 4, 133);

  // battery, bottom-right corner (free of the pos counter and footer hint)
  cardputer::Battery b = cardputer::battery();
  char bs[8];
  if (b.level < 0) snprintf(bs, sizeof bs, "--");
  else snprintf(bs, sizeof bs, "%s%d%%", b.charging ? "+" : "", b.level);
  uint16_t col = b.charging      ? TFT_CYAN
               : b.level < 0     ? d.color565(0x67, 0x88, 0x99)
               : b.level <= 15   ? TFT_RED
               : b.level <= 35   ? d.color565(0xd0, 0xb0, 0x30)
                                 : d.color565(0x67, 0x88, 0x99);
  d.setTextColor(col, TFT_BLACK);
  d.setTextDatum(bottom_right); d.drawString(bs, 236, 133);
}

}  // namespace puz
