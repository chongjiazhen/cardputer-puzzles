#pragma once
#include <Arduino.h>
#include <M5Cardputer.h>   // pulls in M5Unified + M5GFX; provides M5, M5Canvas, M5.Display
#undef PI   // puzzles.h redefines PI with more precision; suppress redefinition warning
extern "C" {
#include "puzzles.h"
}

struct frontend {
  M5Canvas *canvas;    // 8bpp palette sprite; puzzle colours live in palette 0..ncolours-1
  int ncolours;
  bool timer_active;
  bool statusbar;      // game uses a status bar (midend_wants_statusbar); reserves bottom strip
  char status[128];    // status_bar text; sized for win/score strings + timer prefix
  int offX, offY;      // centering offset: puzzle drawn at (offX,offY) on the 240x135 canvas
};

// Reserved palette indices for the frontend's own UI chrome (status strip, canvas
// clear), set once at canvas creation. Puzzle colours fill from index 0 up, so a
// game may use at most 254 (all use far fewer).
static const uint8_t UI_WHITE = 254, UI_BLACK = 255;

extern "C" const drawing_api cardputer_drawing_api;
extern "C" void frontend_load_colours(frontend *fe, midend *me);
