#include <Arduino.h>
#include "cardputer_hw.h"
#include "frontend.h"
#include "presets.h"
#include "menu.h"
#include "keymap.h"
#include "input.h"
#include "pointer.h"
#include "params_ui.h"

static frontend g_fe;
static midend *g_me = nullptr;
static uint32_t g_last_ms;

static puz::Pointer g_ptr{120, 67};
static bool g_ptr_on = false;   // default cursor mode; 'p' toggles the tilt pointer

enum class State { MENU, PLAYING, COMMAND, TYPE_MENU, CONFIG_EDIT, HELP };
static State g_state = State::MENU;
static int g_sel = 0;
static bool g_tab_seen = false;   // self-extinguishing toast flag (Task 9)

// --- callbacks the params_ui screens invoke (bound per game in startGame) ---
static void resumePlaying() { g_state = State::PLAYING; midend_force_redraw(g_me); g_last_ms = millis(); }
static void reloadResumePlaying() {
  frontend_load_colours(&g_fe, g_me);
  int w = 240, h = 135; midend_size(g_me, &w, &h, true, 1.0);
  g_fe.canvas->fillSprite(TFT_BLACK);
  resumePlaying();
}
static void toType()   { g_state = State::TYPE_MENU; }
static void toConfig() { g_state = State::CONFIG_EDIT; }
static void togglePointer() { g_ptr_on = !g_ptr_on; }

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
  g_fe.canvas->fillSprite(TFT_BLACK);   // clear stale pixels from the previous game
  midend_force_redraw(g_me);
  puz::uiBind({ g_me, &reloadResumePlaying, &resumePlaying, &toType, &toConfig, &togglePointer });
  g_state = State::PLAYING;
  g_last_ms = millis();
}

static void openMenu() {
  g_state = State::MENU;
  puz::drawChooser(g_sel);
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

static void drawHelpStub() {
  auto &d = M5.Display;
  d.fillScreen(TFT_BLACK);
  d.setTextSize(1); d.setTextColor(TFT_CYAN, TFT_BLACK);
  d.setTextDatum(top_left); d.drawString("Controls", 4, 2);
  d.drawFastHLine(0, 12, 240, d.color565(0x24, 0x40, 0x55));
  d.setTextColor(TFT_WHITE, TFT_BLACK);
  const char *L[] = {";/. , /  move cursor", "Enter / Space  select", "[ ]  pointer click L / R",
                     "Ctrl+Z / Y  undo / redo", "Ctrl+N / R  new / restart", "Tab  menu (in game)", "`  back"};
  for (int i = 0; i < 7; i++) d.drawString(L[i], 6, 18 + i * 15);
}

static void handlePlaying(puz::InputEvent ev) {
  using puz::Ev;
  switch (ev.kind) {
    case Ev::BackToChooser: openMenu(); return;
    case Ev::CommandMenu:   g_tab_seen = true; g_state = State::COMMAND; puz::openCommand(); return;
    case Ev::Select:        // Enter: pointer-click at crosshair if pointer on, else cursor-select
      if (g_ptr_on) {
        midend_process_key(g_me, (int)g_ptr.x, (int)g_ptr.y, LEFT_BUTTON);
        midend_process_key(g_me, (int)g_ptr.x, (int)g_ptr.y, LEFT_RELEASE);
      } else midend_process_key(g_me, 0, 0, CURSOR_SELECT);
      return;
    case Ev::Select2:       // Space: pointer right-click if pointer on, else cursor-select2
      if (g_ptr_on) {
        midend_process_key(g_me, (int)g_ptr.x, (int)g_ptr.y, RIGHT_BUTTON);
        midend_process_key(g_me, (int)g_ptr.x, (int)g_ptr.y, RIGHT_RELEASE);
      } else midend_process_key(g_me, 0, 0, CURSOR_SELECT2);
      return;
    case Ev::Restart: midend_restart_game(g_me); return;
    case Ev::NewGame:
      midend_new_game(g_me); frontend_load_colours(&g_fe, g_me);
      { int w = 240, h = 135; midend_size(g_me, &w, &h, true, 1.0); }
      midend_force_redraw(g_me); return;
    default: {
      int btn = puz::midendButton(ev);   // cursor/select/select2/char/undo/redo/solve
      if (btn >= 0) midend_process_key(g_me, 0, 0, btn);
      return;
    }
  }
}

void loop() {
  cardputer::update();

  if (g_state == State::MENU) {
    for (auto k : cardputer::keysJustPressedEx()) {
      puz::InputEvent ev = puz::eventForKey(k);
      if (ev.kind == puz::Ev::Up)   { g_sel = (g_sel + gamecount - 1) % gamecount; puz::drawChooser(g_sel); }
      if (ev.kind == puz::Ev::Down) { g_sel = (g_sel + 1) % gamecount;             puz::drawChooser(g_sel); }
      if (ev.kind == puz::Ev::Select)      startGame(g_sel);
      if (ev.kind == puz::Ev::CommandMenu) { g_state = State::HELP; drawHelpStub(); }
    }
    delay(16); return;
  }
  if (g_state == State::HELP) {
    for (auto k : cardputer::keysJustPressedEx())
      if (puz::eventForKey(k).kind == puz::Ev::BackToChooser) openMenu();
    delay(16); return;
  }
  if (g_state == State::COMMAND) {
    for (auto k : cardputer::keysJustPressedEx()) { puz::commandKey(puz::eventForKey(k)); if (g_state != State::COMMAND) break; }
    delay(16); return;
  }
  if (g_state == State::TYPE_MENU) {
    for (auto k : cardputer::keysJustPressedEx()) { puz::typeKey(puz::eventForKey(k)); if (g_state != State::TYPE_MENU) break; }
    delay(16); return;
  }
  if (g_state == State::CONFIG_EDIT) {
    for (auto k : cardputer::keysJustPressedEx()) { puz::configKey(puz::eventForKey(k)); if (g_state != State::CONFIG_EDIT) break; }
    delay(16); return;
  }

  // PLAYING
  uint32_t now = millis();
  float dt = (now - g_last_ms) / 1000.0f;

  cardputer::Imu imu;
  if (g_ptr_on && cardputer::imuRead(imu))
    puz::pointerStep(g_ptr, -imu.ax, imu.ay, dt, 240, 135);  // ADV accel X inverted vs screen

  for (auto k : cardputer::keysJustPressedEx()) {
    if (k.ch == 'p' && !k.ctrl) { g_ptr_on = !g_ptr_on; continue; }  // pointer toggle (PLAYING only)
    handlePlaying(puz::eventForKey(k));
    if (g_state != State::PLAYING) return;   // a handler changed state
  }

  if (g_fe.timer_active) midend_timer(g_me, dt);
  midend_redraw(g_me);

  if (g_ptr_on) {
    int x = (int)g_ptr.x, y = (int)g_ptr.y;
    M5.Display.drawLine(x - 4, y, x + 4, y, TFT_RED);
    M5.Display.drawLine(x, y - 4, x, y + 4, TFT_RED);
  }

  g_last_ms = now;
  delay(16);
}
