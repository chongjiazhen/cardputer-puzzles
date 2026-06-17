#include <Arduino.h>
#include "cardputer_hw.h"
extern "C" {
#undef PI   // puzzles.h redefines PI with more precision; suppress redefinition warning
#include "puzzles.h"
}

extern "C" const drawing_api stub_drawing_api;
// gamelist[] and gamecount are declared by puzzles.h under #ifdef COMBINED

struct frontend { int unused; };
static frontend g_fe;
static midend *g_me;

void setup() {
  Serial.begin(115200);
  cardputer::begin();
  g_me = midend_new(&g_fe, gamelist[0], &stub_drawing_api, &g_fe);
  midend_new_game(g_me);
  int w = 240, h = 135;
  midend_size(g_me, &w, &h, true, 1.0);
  Serial.printf("game=%s sized w=%d h=%d gamecount=%d\n",
                gamelist[0]->name, w, h, gamecount);
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setCursor(0, 0);
  M5.Display.printf("%s %dx%d", gamelist[0]->name, w, h);
}

void loop() { cardputer::update(); delay(50); }
