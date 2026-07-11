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
  bool zoom;           // 2x magnifier mode: frame pushed magnified around (zoomX,zoomY)
  int zoomX, zoomY;    // zoom pan centre, canvas coords
};

// Reserved palette indices for the frontend's own UI chrome (status strip, canvas
// clear, pointer crosshair), set once at canvas creation. Puzzle colours fill from
// index 0 up, so a game may use at most 252 (all use far fewer).
static const uint8_t UI_RED = 252, UI_BLUE = 253, UI_WHITE = 254, UI_BLACK = 255;

// Firmware version, shown on the crash screen and the Help footer so a tester's
// screenshot of a crash is a self-contained bug report. Bump per release.
#define FW_VERSION "v1.1"

extern "C" const drawing_api cardputer_drawing_api;
extern "C" void frontend_load_colours(frontend *fe, midend *me);
// Push the canvas to the panel — full-screen normally, 2x magnified around
// (zoomX,zoomY) when fe->zoom. d_end uses this; pan handlers call it directly
// to re-push without forcing a game redraw.
extern "C" void frontend_push(frontend *fe);
// Record the current game + its game-id so fatal() can show it on a crash.
// params may be NULL (-> "default"). Call before generating, then again with the
// resolved game-id once a board exists.
extern "C" void frontend_set_crash(const char *game, const char *params);
