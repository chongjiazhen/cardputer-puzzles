#pragma once
#include <Arduino.h>
#include <M5Cardputer.h>   // pulls in M5Unified + M5GFX; provides M5, M5Canvas, M5.Display
#undef PI   // puzzles.h redefines PI with more precision; suppress redefinition warning
extern "C" {
#include "puzzles.h"
}

struct frontend {
  M5Canvas *canvas;
  uint16_t *colours;   // 565, length ncolours
  int ncolours;
  bool timer_active;
  bool statusbar;      // game uses a status bar (midend_wants_statusbar); reserves bottom strip
  char status[64];
  int offX, offY;      // centering offset: puzzle drawn at (offX,offY) on the 240x135 canvas
};

extern "C" const drawing_api cardputer_drawing_api;
extern "C" void frontend_load_colours(frontend *fe, midend *me);
