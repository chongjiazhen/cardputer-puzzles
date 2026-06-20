#include <M5Cardputer.h>
#include "picker.h"

namespace puz {

void drawPicker(const char* const* items, int n, int sel,
                const char* title, const char* pos,
                const char* const* suffix) {
  auto &d = M5.Display;
  d.fillScreen(TFT_BLACK);
  d.setTextSize(1);

  // header
  d.setTextColor(TFT_CYAN, TFT_BLACK);
  d.setTextDatum(top_left);  d.drawString(title, 4, 2);
  if (pos) { d.setTextDatum(top_right); d.drawString(pos, 236, 2); }
  d.drawFastHLine(0, 12, 240, d.color565(0x24, 0x40, 0x55));

  PickerWindow w = pickerWindow(n, sel, 7);
  const int centerY = 74, rowH = 15;
  for (int s = 0; s < w.count; s++) {
    int i = w.idx[s];
    int dy = s - w.selSlot;
    int y = centerY + dy * rowH;
    if (dy == 0) {
      d.fillRoundRect(2, y - 7, 236, 15, 2, TFT_WHITE);
      d.setTextColor(TFT_BLACK, TFT_WHITE);
    } else {
      int dist = dy < 0 ? -dy : dy;
      uint8_t g = dist == 1 ? 0x9a : dist == 2 ? 0x5e : 0x3a;
      d.setTextColor(d.color565(g, g, g), TFT_BLACK);
    }
    d.setTextDatum(middle_left);
    d.drawString(items[i], 8, y);
    if (suffix && suffix[i] && suffix[i][0]) {
      d.setTextDatum(middle_right);
      d.drawString(suffix[i], 232, y);
    }
  }
}

}  // namespace puz
