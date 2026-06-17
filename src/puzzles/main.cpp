#include <Arduino.h>
#include "cardputer_hw.h"
#include "frontend.h"
#include "keymap.h"
#include "input.h"
#include "pointer.h"

// gamelist[] and gamecount declared by puzzles.h under #ifdef COMBINED (included via frontend.h)

static frontend g_fe;
static midend *g_me;
static uint32_t g_last_ms;

static puz::Pointer g_ptr{120, 67};
static bool g_ptr_on = true;

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

  // Single dt computed once; used for both IMU step and timer.
  uint32_t now = millis();
  float dt = (now - g_last_ms) / 1000.0f;

  // IMU: advance pointer when enabled.
  cardputer::Imu imu;
  if (g_ptr_on && cardputer::imuRead(imu))
    puz::pointerStep(g_ptr, imu.ax, imu.ay, dt, 240, 135);

  // Input: map each pressed key to an event and process it.
  for (char c : cardputer::keysJustPressed()) {
    puz::InputEvent ev = puz::eventForChar(c);

    // Pointer-coord events handled before midendButton (they return -1).
    if (ev.kind == puz::Ev::TogglePointer) {
      g_ptr_on = !g_ptr_on;
      continue;
    }
    if (ev.kind == puz::Ev::ClickL) {
      midend_process_key(g_me, (int)g_ptr.x, (int)g_ptr.y, LEFT_BUTTON);
      midend_process_key(g_me, (int)g_ptr.x, (int)g_ptr.y, LEFT_RELEASE);
      continue;
    }
    if (ev.kind == puz::Ev::ClickR) {
      midend_process_key(g_me, (int)g_ptr.x, (int)g_ptr.y, RIGHT_BUTTON);
      midend_process_key(g_me, (int)g_ptr.x, (int)g_ptr.y, RIGHT_RELEASE);
      continue;
    }

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
  if (g_fe.timer_active) {
    midend_timer(g_me, dt);
  }

  // Redraw: midend only repaints changed regions internally.
  midend_redraw(g_me);

  // Cursor: draw red crosshair on M5.Display (not canvas) so blitter never captures it.
  if (g_ptr_on) {
    int x = (int)g_ptr.x, y = (int)g_ptr.y;
    M5.Display.drawLine(x - 4, y, x + 4, y, TFT_RED);
    M5.Display.drawLine(x, y - 4, x, y + 4, TFT_RED);
  }

  g_last_ms = now;

  delay(16);
}
