#include <Arduino.h>
#include "cardputer_hw.h"
#include "frontend.h"

// gamelist[] and gamecount declared by puzzles.h under #ifdef COMBINED (included via frontend.h)

static frontend g_fe;
static midend *g_me;

void setup() {
  Serial.begin(115200);
  cardputer::begin();

  g_fe.canvas = new M5Canvas(&M5.Display);
  if (!g_fe.canvas->createSprite(240, 135))
    fatal("canvas createSprite failed - not enough SRAM");
  g_fe.colours = nullptr; g_fe.ncolours = 0; g_fe.timer_active = false;
  g_fe.status[0] = '\0';

  g_me = midend_new(&g_fe, gamelist[0], &cardputer_drawing_api, &g_fe);
  midend_new_game(g_me);
  frontend_load_colours(&g_fe, g_me);

  int w = 240, h = 135;
  midend_size(g_me, &w, &h, true, 1.0);
  Serial.printf("game=%s sized w=%d h=%d gamecount=%d\n",
                gamelist[0]->name, w, h, gamecount);

  midend_force_redraw(g_me);
}

void loop() { cardputer::update(); delay(50); }
