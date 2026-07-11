#include <M5Cardputer.h>
#include "picker.h"

namespace puz {

void drawPicker(const char* const* items, int n, int sel,
                const char* title, const char* pos,
                const char* const* suffix, int maxRows) {
  auto &d = M5.Display;
  d.fillScreen(TFT_BLACK);
  d.setTextSize(1);

  // header
  d.setTextColor(TFT_CYAN, TFT_BLACK);
  d.setTextDatum(top_left);  d.drawString(title, 4, 2);
  if (pos) { d.setTextDatum(top_right); d.drawString(pos, 236, 2); }
  d.drawFastHLine(0, 12, 240, d.color565(0x24, 0x40, 0x55));

  PickerWindow w = pickerWindow(n, sel, maxRows);
  // Spread the visible rows across the band between header (16) and footer (~128).
  const int top = 16, bot = 128, centerY = (top + bot) / 2;
  int rowH = (bot - top) / w.count; if (rowH > 24) rowH = 24;
  const int hl = rowH - 2;                       // highlight bar height
  const int selSize = (rowH >= 20) ? 2 : 1;      // bigger selected text when rows are tall enough
  for (int s = 0; s < w.count; s++) {
    int i = w.idx[s];
    int dy = s - w.selSlot;
    int y = centerY + dy * rowH;
    if (dy == 0) {
      d.fillRoundRect(2, y - hl / 2, 236, hl, 2, TFT_WHITE);
      d.setTextColor(TFT_BLACK, TFT_WHITE);
    } else {
      int dist = dy < 0 ? -dy : dy;
      uint8_t g = dist == 1 ? 0x9a : dist == 2 ? 0x5e : 0x3a;
      d.setTextColor(d.color565(g, g, g), TFT_BLACK);
    }
    d.setTextSize(dy == 0 ? selSize : 1);
    d.setTextDatum(middle_left);
    d.drawString(items[i], 8, y);
    if (suffix && suffix[i] && suffix[i][0]) {
      d.setTextSize(1);
      d.setTextDatum(middle_right);
      d.drawString(suffix[i], 228, y);   // left of the scrollbar gutter
    }
  }
  d.setTextSize(1);

  // Scrollbar: only when the list overflows the visible window. Thumb size
  // tracks the visible fraction, position tracks sel. Cyan so it reads over
  // both the white selected bar and the black background.
  if (n > maxRows) {
    const int trackTop = 16, trackBot = 128, trackH = trackBot - trackTop;
    const int sbx = 237;
    d.drawFastVLine(sbx, trackTop, trackH, d.color565(0x24, 0x40, 0x55));
    int thumbH = trackH * maxRows / n; if (thumbH < 10) thumbH = 10;
    int thumbY = trackTop + (trackH - thumbH) * sel / (n - 1);
    d.fillRect(sbx - 1, thumbY, 3, thumbH, TFT_CYAN);
  }
}

}  // namespace puz
