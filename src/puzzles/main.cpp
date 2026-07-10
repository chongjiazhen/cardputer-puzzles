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
static const char *g_curGameName = "-";   // current game name, for the crash-report context
static int g_curIdx = -1;                 // currently-loaded game; re-entering it resumes in-progress

enum class State { MENU, PLAYING, COMMAND, TYPE_MENU, CONFIG_EDIT, HELP, RULES };
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
  { char *id = midend_get_game_id(g_me);   // params changed (preset/custom) -> refresh crash ctx
    frontend_set_crash(g_curGameName, id); sfree(id); }
  g_fe.canvas->fillSprite(UI_BLACK);
  resumePlaying();
}
static void toType()   { g_state = State::TYPE_MENU; }
static void toConfig() { g_state = State::CONFIG_EDIT; }
static void toRules()    { g_state = State::RULES; }
static void togglePointer() { g_ptr_on = !g_ptr_on; }

// --- zoom (Ctrl+L): live 2x magnifier, game keeps playing ---
// d_end pushes the frame magnified around (zoomX,zoomY) while fe.zoom is set
// (see frontend_push), so every game redraw stays zoomed. Ctrl+;,./ pans;
// everything else reaches the game as usual.
static void clampZoom() {
  // Keep the 120x68 source window inside the 240x135 canvas.
  if (g_fe.zoomX < 60)  g_fe.zoomX = 60;  if (g_fe.zoomX > 180) g_fe.zoomX = 180;
  if (g_fe.zoomY < 34)  g_fe.zoomY = 34;  if (g_fe.zoomY > 101) g_fe.zoomY = 101;
}
static void toggleZoom() {
  g_fe.zoom = !g_fe.zoom;
  g_fe.zoomX = 120; g_fe.zoomY = 67;
  frontend_push(&g_fe);   // repaint immediately at the new magnification
}
static void panZoom(int dx, int dy) {
  if (!g_fe.zoom) return;
  g_fe.zoomX += dx; g_fe.zoomY += dy; clampZoom();
  frontend_push(&g_fe);
}
// Screen -> puzzle coords for pointer clicks, aware of the magnified view:
// screen pixel s shows canvas pixel zoomCentre + (s - screenCentre)/2.
static void ptrToPuzzle(int *px, int *py) {
  int sx = (int)g_ptr.x, sy = (int)g_ptr.y;
  if (g_fe.zoom) { sx = g_fe.zoomX + (sx - 120) / 2; sy = g_fe.zoomY + (sy - 67) / 2; }
  *px = sx - g_fe.offX; *py = sy - g_fe.offY;
}

// --- pointer-mode direct digit entry ---
// Digit-entry games (keen, solo, towers, ...) only accept a digit on a cell
// selected by a prior click, and hide the selection again after each digit
// (mouse-mode semantics in interpret_move). Requiring Enter before every digit
// is miserable on a handheld, so in pointer mode a digit auto-left-clicks the
// crosshair cell first. We track whether a selection is already active at the
// crosshair (from an explicit Enter/Space click) so that:
//  - Space (right-click) then digit gives PENCIL marks — no auto-left-click
//    stomping the pencil highlight;
//  - Enter then digit doesn't re-click (a second left-click on the selected
//    cell would toggle the selection OFF and swallow the digit).
enum class PtrSel { None, Fill, Pencil };
static PtrSel g_ptrSel = PtrSel::None;
static bool g_ptrHl = false;   // a click-made highlight is still showing (no digit since)
static float g_selPX = 0, g_selPY = 0;
static bool ptrNearSel() {
  // Only used to mirror the games' click-on-selected-cell toggle. Radius ~half
  // a typical tile; the IMU crosshair drifts a few px/s, so keep it generous.
  float dx = g_ptr.x - g_selPX, dy = g_ptr.y - g_selPY;
  return dx * dx + dy * dy < 144;   // within ~12px of the click
}
// Mirror the games' click-toggle: clicking the selected cell again deselects.
static void ptrTrackClick(PtrSel mode) {
  if (g_ptrSel == mode && g_ptrHl && ptrNearSel()) { g_ptrSel = PtrSel::None; g_ptrHl = false; }
  else { g_ptrSel = mode; g_ptrHl = true; g_selPX = g_ptr.x; g_selPY = g_ptr.y; }
}

static void startGame(int idx) {
  if (g_me) { midend_free(g_me); g_me = nullptr; }
  g_me = midend_new(&g_fe, gamelist[idx], &cardputer_drawing_api, &g_fe);
  if (!g_me) return;
  g_curIdx = idx;
  g_curGameName = gamelist[idx]->name;
  g_fe.zoom = false;   // fresh game starts unmagnified
  const char *p = presetFor(g_curGameName);
  if (p) midend_game_id(g_me, p);  // returns nullptr on success; on error params stay default
  frontend_set_crash(g_curGameName, p);   // crash ctx before generation (name + preset/default)
  midend_new_game(g_me);
  { char *id = midend_get_game_id(g_me);   // refine with the resolved game-id (desc exists now)
    frontend_set_crash(g_curGameName, id); sfree(id); }
  frontend_load_colours(&g_fe, g_me);
  sizeAndCenter();
  g_fe.canvas->fillSprite(UI_BLACK);   // clear stale pixels from the previous game
  midend_force_redraw(g_me);
  puz::uiBind({ g_me, &reloadResumePlaying, &resumePlaying, &toType, &toConfig, &toRules, &togglePointer, &toggleZoom });
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

// Selecting a game from the chooser: if it's the game already loaded, resume it
// in progress (so an accidental ` back to the menu doesn't lose a puzzle) instead
// of regenerating. Switching to a different title starts that one fresh.
static void enterGame(int idx) {
  if (g_me && idx == g_curIdx) { resumePlaying(); return; }
  startGame(idx);
}

void setup() {
  Serial.begin(115200);
  cardputer::begin();

  // Start every cursor game with its keyboard cursor already visible. Without
  // this, upstream inits cur_visible=false (getenv_bool default) and the FIRST
  // CURSOR_SELECT only reveals the cursor at region 0 instead of acting -- so
  // Enter/Space feel dead / off-by-one until an arrow or a wasted press wakes
  // it. 35 games honour this flag (incl. Map's pipette pickup). Value must lead
  // with y/Y/t/T -- getenv_bool (upstream misc.c) tests only the first char.
  setenv("PUZZLES_SHOW_CURSOR", "y", 1);

  g_fe.canvas = new M5Canvas(&M5.Display);
  g_fe.canvas->setColorDepth(lgfx::color_depth_t::palette_8bit);   // 8bpp palette: 32KB vs 64KB at 16bpp
                                                                   // (plain 8 = rgb332, no palette -> wrong colours)
  if (!g_fe.canvas->createSprite(240, 135))   // auto-allocates the 256-entry palette
    fatal("canvas createSprite failed - not enough SRAM");
  g_fe.canvas->setPaletteColor(UI_WHITE, 255, 255, 255);   // reserved UI chrome
  g_fe.canvas->setPaletteColor(UI_BLACK, 0, 0, 0);
  g_fe.canvas->setPaletteColor(UI_RED, 255, 0, 0);         // pointer crosshair (fill)
  g_fe.canvas->setPaletteColor(UI_BLUE, 0, 0, 255);        // pointer crosshair (pencil)
  g_fe.ncolours = 0; g_fe.timer_active = false;
  g_fe.statusbar = false;
  g_fe.status[0] = '\0';
  g_fe.zoom = false; g_fe.zoomX = 120; g_fe.zoomY = 67;

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
                     "Tab         menu (in game)", "Ctrl+P      tilt pointer",
                     "Ctrl+L zoom  Ctrl+;,./ pan"};
  for (int i = 0; i < 7; i++) d.drawString(L[i], 6, 20 + i * 16);
  d.setTextColor(d.color565(0x67, 0x88, 0x99), TFT_BLACK);
  d.setTextDatum(bottom_left);  d.drawString("` back", 4, 133);
  d.setTextDatum(bottom_right); d.drawString(FW_VERSION, 236, 133);
}

static void handlePlaying(puz::InputEvent ev) {
  using puz::Ev;
  switch (ev.kind) {
    case Ev::BackToChooser: openMenu(); return;
    case Ev::CommandMenu:   g_tab_seen = true; g_state = State::COMMAND; puz::openCommand(); return;
    case Ev::Select:        // Enter: pointer-click at crosshair if pointer on, else cursor-select
      if (g_ptr_on) {
        int px, py; ptrToPuzzle(&px, &py);   // screen -> puzzle coords (zoom-aware)
        midend_process_key(g_me, px, py, LEFT_BUTTON);
        midend_process_key(g_me, px, py, LEFT_RELEASE);
        ptrTrackClick(PtrSel::Fill);
      } else midend_process_key(g_me, 0, 0, CURSOR_SELECT);
      return;
    case Ev::Select2:       // Space: pointer right-click if pointer on, else cursor-select2
      if (g_ptr_on) {
        int px, py; ptrToPuzzle(&px, &py);
        midend_process_key(g_me, px, py, RIGHT_BUTTON);
        midend_process_key(g_me, px, py, RIGHT_RELEASE);
        ptrTrackClick(PtrSel::Pencil);
      } else midend_process_key(g_me, 0, 0, CURSOR_SELECT2);
      return;
    case Ev::Restart: midend_restart_game(g_me); return;
    case Ev::TogglePointer: g_ptr_on = !g_ptr_on; g_ptrSel = PtrSel::None; g_ptrHl = false; return;   // Ctrl+P (plain 'p' belongs to the game)
    case Ev::ZoomPeek: toggleZoom(); return;                // Ctrl+L: toggle live 2x magnifier
    case Ev::PanUp:    panZoom(0, -16); return;             // Ctrl+;,./ pan the zoom window
    case Ev::PanDown:  panZoom(0,  16); return;
    case Ev::PanLeft:  panZoom(-16, 0); return;
    case Ev::PanRight: panZoom( 16, 0); return;
    case Ev::NewGame:
      midend_new_game(g_me); frontend_load_colours(&g_fe, g_me);
      sizeAndCenter();
      midend_force_redraw(g_me); return;
    default: {
      // Pointer-mode direct entry: a digit (or backspace = clear) lands on the
      // crosshair cell without a prior Enter. Auto-left-click first unless a
      // selection is already active there (explicit Enter/Space click) — see
      // PtrSel above. Only digits/backspace: letters can be global commands in
      // some games (Map's 'l', Undead's monsters), where a stray click moves.
      if (g_ptr_on && (ev.kind == puz::Ev::Char) &&
          ((ev.ch >= '0' && ev.ch <= '9') || ev.ch == '\b')) {
        if (g_ptrHl) {
          // A click-made highlight is still showing: send the digit straight to
          // it. The game keyed the selection to the clicked CELL, so crosshair
          // drift since the click doesn't matter — and an extra click here
          // would toggle the selection off / stomp a pencil highlight.
          midend_process_key(g_me, 0, 0, (unsigned char)ev.ch);
          g_ptrHl = false;                              // game hides it after the digit
          if (g_ptrSel == PtrSel::Fill) g_ptrSel = PtrSel::None;
          // Pencil stays armed: see below.
        } else {
          // No live highlight: click first, then the digit. Pencil mode is
          // sticky — Space arms it, and each digit re-right-clicks so marks
          // keep appending (games hide the pencil highlight after every digit;
          // pencil_keep_highlight defaults off upstream). Enter/Space exits.
          int px, py; ptrToPuzzle(&px, &py);
          int btn = (g_ptrSel == PtrSel::Pencil) ? RIGHT_BUTTON : LEFT_BUTTON;
          int rel = (g_ptrSel == PtrSel::Pencil) ? RIGHT_RELEASE : LEFT_RELEASE;
          midend_process_key(g_me, px, py, btn);
          midend_process_key(g_me, px, py, rel);
          midend_process_key(g_me, 0, 0, (unsigned char)ev.ch);
        }
        return;
      }
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
      if (ev.kind == puz::Ev::Select)      enterGame(g_sel);
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
  if (g_state == State::RULES) {
    for (auto k : cardputer::keysJustPressedEx()) { puz::ruleKey(puz::eventForKey(k)); if (g_state != State::RULES) break; }
    delay(16); return;
  }

  // PLAYING
  uint32_t now = millis();
  float dt = (now - g_last_ms) / 1000.0f;

  cardputer::Imu imu;
  if (g_ptr_on && cardputer::imuRead(imu)) {
    puz::pointerStep(g_ptr, -imu.ax, imu.ay, dt, 240, 135);  // ADV accel X inverted vs screen
    if (g_fe.zoom) {
      // Edge-pan: crosshair pushed against a screen edge drags the zoom window
      // along, so the pointer can reach the whole board without Ctrl+panning.
      const int margin = 8, speed = 3;
      int dx = 0, dy = 0;
      if (g_ptr.x < margin) dx = -speed; else if (g_ptr.x > 240 - margin) dx = speed;
      if (g_ptr.y < margin) dy = -speed; else if (g_ptr.y > 135 - margin) dy = speed;
      if (dx || dy) { g_fe.zoomX += dx; g_fe.zoomY += dy; clampZoom(); }
      // (no push here: the pointer-mode push below repaints every frame)
    }
  }

  for (auto k : cardputer::keysJustPressedEx()) {
    handlePlaying(puz::eventForKey(k));   // Ctrl+P → Ev::TogglePointer, handled in handlePlaying
    if (g_state != State::PLAYING) return;   // a handler changed state
  }

  if (g_fe.timer_active) midend_timer(g_me, dt);
  midend_redraw(g_me);

  if (g_ptr_on) {
    // Composite the crosshair INTO the canvas so it pushes atomically with the
    // frame — drawing straight to the panel after the push both flickers (the
    // next push overwrites it for a few ms) and smears (pushRotateZoom doesn't
    // repaint the outermost rows, so panel-drawn pixels survive at the edges).
    // Save-draw-push-restore, same trick as the midend blitter.
    int cx = (int)g_ptr.x, cy = (int)g_ptr.y;
    if (g_fe.zoom) {   // screen -> canvas coords; magnification bolds it back up
      cx = g_fe.zoomX + (cx - 120) / 2;
      cy = g_fe.zoomY + (cy - 67) / 2;
    }
    if (cx < 4) cx = 4; if (cx > 235) cx = 235;   // keep the 9x9 patch on-canvas
    if (cy < 4) cy = 4; if (cy > 130) cy = 130;
    // Save/restore per-pixel via readPixelValue/drawPixel: those speak raw
    // palette indices. readRect/pushImage do NOT round-trip indices on a
    // palette sprite (they colour-convert), which corrupts the board.
    static uint8_t saved[9 * 9];
    M5Canvas *cv = g_fe.canvas;
    for (int dy = 0; dy < 9; dy++)
      for (int dx = 0; dx < 9; dx++)
        saved[dy * 9 + dx] = (uint8_t)cv->readPixelValue(cx - 4 + dx, cy - 4 + dy);
    if (g_ptrSel == PtrSel::Pencil) {
      // Pencil mode: blue diagonal X so the mode is visible at a glance.
      cv->drawLine(cx - 3, cy - 3, cx + 3, cy + 3, UI_BLUE);
      cv->drawLine(cx - 3, cy + 3, cx + 3, cy - 3, UI_BLUE);
    } else {
      cv->drawLine(cx - 4, cy, cx + 4, cy, UI_RED);
      cv->drawLine(cx, cy - 4, cx, cy + 4, UI_RED);
    }
    frontend_push(&g_fe);
    for (int dy = 0; dy < 9; dy++)   // restore the game's pixels
      for (int dx = 0; dx < 9; dx++)
        cv->drawPixel(cx - 4 + dx, cy - 4 + dy, saved[dy * 9 + dx]);
  }

  g_last_ms = now;
  delay(16);
}
