#include <Arduino.h>
#include "cardputer_hw.h"
#include "frontend.h"
#include "keymap.h"
#include "input.h"

// gamelist[] and gamecount declared by puzzles.h under #ifdef COMBINED (included via frontend.h)

static frontend g_fe;
static midend *g_me;
static uint32_t g_last_ms;

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
  g_last_ms = millis();
}

void loop() {
  cardputer::update();

  // Input: map each pressed key to a midend button and process it.
  for (char c : cardputer::keysJustPressed()) {
    puz::InputEvent ev = puz::eventForChar(c);
    if (ev.kind == puz::Ev::Menu) {
      // Task 8: return to chooser menu. No-op for now.
      continue;
    }
    int btn = puz::midendButton(ev);
    if (btn >= 0) {
      if (midend_process_key(g_me, 0, 0, btn) == PKR_QUIT) {
        // Task 8: PKR_QUIT will trigger chooser / shutdown. No-op for now.
      }
    }
  }

  // Timer: advance midend clock when a timed animation is active.
  uint32_t now = millis();
  if (g_fe.timer_active) {
    midend_timer(g_me, (now - g_last_ms) / 1000.0f);
  }
  g_last_ms = now;

  // Redraw: midend only repaints changed regions internally.
  midend_redraw(g_me);

  delay(16);
}
