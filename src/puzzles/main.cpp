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
static bool g_ptr_on = false;   // default cursor mode; Ctrl+P toggles the tilt pointer

enum class State { MENU, PLAYING, COMMAND, TYPE_MENU, CONFIG_EDIT, HELP };
static State g_state = State::MENU;
static int g_sel = 0;
static bool g_tab_seen = false;   // self-extinguishing splash flag (set on first Tab)

// --- callbacks the params_ui screens invoke (bound per game in startGame) ---
static void resumePlaying() { g_state = State::PLAYING; midend_force_redraw(g_me); g_last_ms = millis(); }
// Recompute board size and re-center on the 240x135 canvas. Must run after any
// midend_size-changing event (new game, preset/config apply); otherwise offX/offY
// go stale and both rendering and pointer-click translation use the old centering.
static void sizeAndCenter() {
  // Reserve an 11px bottom strip for the status bar on games that use one, so the
  // board centers above it instead of being overdrawn by the overlay (see d_end).
  // Games with no status bar keep the full 135px height. Clear any status text left
  // over from a previous game so it can't bleed onto a no-statusbar board.
  g_fe.statusbar = midend_wants_statusbar(g_me);
  g_fe.status[0] = '\0';
  int avail = g_fe.statusbar ? 135 - 11 : 135;
  int w = 240, h = avail;
  midend_size(g_me, &w, &h, true, 1.0);
  g_fe.offX = (240 - w) / 2; g_fe.offY = (avail - h) / 2;
}
static void reloadResumePlaying() {
  frontend_load_colours(&g_fe, g_me);
  sizeAndCenter();
  g_fe.canvas->fillSprite(UI_BLACK);
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
  sizeAndCenter();
  g_fe.canvas->fillSprite(UI_BLACK);   // clear stale pixels from the previous game
  midend_force_redraw(g_me);
  puz::uiBind({ g_me, &reloadResumePlaying, &resumePlaying, &toType, &toConfig, &togglePointer });
  // One-shot discoverability splash: shown on the first game entry per boot, then never again.
  if (!g_tab_seen) {
    auto &d = M5.Display;
    d.fillScreen(TFT_BLACK);
    d.setTextSize(2); d.setTextColor(TFT_CYAN, TFT_BLACK); d.setTextDatum(middle_center);
    d.drawString("Tab = menu", 120, 58);
    d.setTextSize(1); d.setTextColor(d.color565(0x88, 0x99, 0xaa), TFT_BLACK);
    d.drawString("options, size, undo...", 120, 82);
    delay(1100);
    g_tab_seen = true;   // shown once; don't nag again this session
  }
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
  g_fe.canvas->setColorDepth(lgfx::color_depth_t::palette_8bit);   // 8bpp palette: 32KB vs 64KB at 16bpp
                                                                   // (plain 8 = rgb332, no palette -> wrong colours)
  if (!g_fe.canvas->createSprite(240, 135))   // auto-allocates the 256-entry palette
    fatal("canvas createSprite failed - not enough SRAM");
  g_fe.canvas->setPaletteColor(UI_WHITE, 255, 255, 255);   // reserved UI chrome
  g_fe.canvas->setPaletteColor(UI_BLACK, 0, 0, 0);
  g_fe.ncolours = 0; g_fe.timer_active = false;
  g_fe.statusbar = false;
  g_fe.status[0] = '\0';

  openMenu();
}

static void drawHelp() {
  auto &d = M5.Display;
  d.fillScreen(TFT_BLACK);
  d.setTextSize(1); d.setTextColor(TFT_CYAN, TFT_BLACK);
  d.setTextDatum(top_left); d.drawString("Controls", 4, 2);
  d.drawFastHLine(0, 12, 240, d.color565(0x24, 0x40, 0x55));
  d.setTextColor(TFT_WHITE, TFT_BLACK);
  const char *L[] = {";,./        move cursor", "Enter/Space select / mark",
                     "Ctrl+Z / Y  undo / redo", "Ctrl+N / R  new / restart",
                     "Tab         menu (in game)", "Ctrl+P      tilt pointer"};
  for (int i = 0; i < 6; i++) d.drawString(L[i], 6, 20 + i * 16);
  d.setTextColor(d.color565(0x67, 0x88, 0x99), TFT_BLACK);
  d.setTextDatum(bottom_left); d.drawString("` back", 4, 133);
}

static void handlePlaying(puz::InputEvent ev) {
  using puz::Ev;
  switch (ev.kind) {
    case Ev::BackToChooser: openMenu(); return;
    case Ev::CommandMenu:   g_tab_seen = true; g_state = State::COMMAND; puz::openCommand(); return;
    case Ev::Select:        // Enter: pointer-click at crosshair if pointer on, else cursor-select
      if (g_ptr_on) {
        int px = (int)g_ptr.x - g_fe.offX, py = (int)g_ptr.y - g_fe.offY;  // screen -> puzzle coords
        midend_process_key(g_me, px, py, LEFT_BUTTON);
        midend_process_key(g_me, px, py, LEFT_RELEASE);
      } else midend_process_key(g_me, 0, 0, CURSOR_SELECT);
      return;
    case Ev::Select2:       // Space: pointer right-click if pointer on, else cursor-select2
      if (g_ptr_on) {
        int px = (int)g_ptr.x - g_fe.offX, py = (int)g_ptr.y - g_fe.offY;
        midend_process_key(g_me, px, py, RIGHT_BUTTON);
        midend_process_key(g_me, px, py, RIGHT_RELEASE);
      } else midend_process_key(g_me, 0, 0, CURSOR_SELECT2);
      return;
    case Ev::Restart: midend_restart_game(g_me); return;
    case Ev::TogglePointer: g_ptr_on = !g_ptr_on; return;   // Ctrl+P (plain 'p' belongs to the game)
    case Ev::NewGame:
      midend_new_game(g_me); frontend_load_colours(&g_fe, g_me);
      sizeAndCenter();
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
      if (ev.kind == puz::Ev::CommandMenu) { g_state = State::HELP; drawHelp(); }
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
    handlePlaying(puz::eventForKey(k));   // Ctrl+P → Ev::TogglePointer, handled in handlePlaying
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
