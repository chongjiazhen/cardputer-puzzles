#include <Arduino.h>
#include "cardputer_hw.h"
#include "frontend.h"
#include "presets.h"
#include "menu.h"
#include "keymap.h"
#include "input.h"
#include "pointer.h"

static frontend g_fe;
static midend *g_me = nullptr;
static uint32_t g_last_ms;

static puz::Pointer g_ptr{120, 67};
static bool g_ptr_on = true;

enum class State { MENU, PLAYING };
static State g_state = State::MENU;
static int g_sel = 0;

static void startGame(int idx) {
  if (g_me) { midend_free(g_me); g_me = nullptr; }
  g_me = midend_new(&g_fe, gamelist[idx], &cardputer_drawing_api, &g_fe);
  if (!g_me) return;
  if (const char *p = presetFor(gamelist[idx]->name))
    midend_game_id(g_me, p);  // returns nullptr on success; on error params stay default
  midend_new_game(g_me);
  frontend_load_colours(&g_fe, g_me);
  int w = 240, h = 135;
  midend_size(g_me, &w, &h, true, 1.0);
  midend_force_redraw(g_me);
  g_state = State::PLAYING;
  g_last_ms = millis();
}

static void openMenu() {
  g_state = State::MENU;
  puz::drawMenu(g_sel);
}

void setup() {
  Serial.begin(115200);
  cardputer::begin();

  g_fe.canvas = new M5Canvas(&M5.Display);
  if (!g_fe.canvas->createSprite(240, 135))
    fatal("canvas createSprite failed - not enough SRAM");
  g_fe.colours = nullptr; g_fe.ncolours = 0; g_fe.timer_active = false;
  g_fe.status[0] = '\0';

  openMenu();
}

void loop() {
  cardputer::update();

  if (g_state == State::MENU) {
    for (char c : cardputer::keysJustPressed()) {
      puz::InputEvent ev = puz::eventForChar(c);
      if (ev.kind == puz::Ev::Up)   { g_sel = (g_sel + gamecount - 1) % gamecount; puz::drawMenu(g_sel); }
      if (ev.kind == puz::Ev::Down) { g_sel = (g_sel + 1) % gamecount;             puz::drawMenu(g_sel); }
      if (ev.kind == puz::Ev::Select) startGame(g_sel);
    }
    delay(16);
    return;
  }

  uint32_t now = millis();
  float dt = (now - g_last_ms) / 1000.0f;

  cardputer::Imu imu;
  if (g_ptr_on && cardputer::imuRead(imu))
    puz::pointerStep(g_ptr, -imu.ax, imu.ay, dt, 240, 135);  // ADV accel X is inverted vs screen

  for (char c : cardputer::keysJustPressed()) {
    puz::InputEvent ev = puz::eventForChar(c);

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
      openMenu();
      return;
    }
    int btn = puz::midendButton(ev);
    if (btn >= 0) {
      if (midend_process_key(g_me, 0, 0, btn) == PKR_QUIT) {
        openMenu();
        return;
      }
    }
  }

  if (g_fe.timer_active) {
    midend_timer(g_me, dt);
  }

  midend_redraw(g_me);

  if (g_ptr_on) {
    int x = (int)g_ptr.x, y = (int)g_ptr.y;
    M5.Display.drawLine(x - 4, y, x + 4, y, TFT_RED);
    M5.Display.drawLine(x, y - 4, x, y + 4, TFT_RED);
  }

  g_last_ms = now;

  delay(16);
}
