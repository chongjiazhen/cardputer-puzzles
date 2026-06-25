#include "frontend.h"

// BLITTER_FROMSAVED: sentinel not defined in upstream puzzles.h; define locally.
// Callers (game backends) pass this to reuse the saved x/y. Net does not use blitters.
#ifndef BLITTER_FROMSAVED
#define BLITTER_FROMSAVED (-1)
#endif

// ---- colour palette ----
// The canvas is an 8bpp palette sprite: every draw primitive passes the puzzle's
// colour INDEX (0..ncolours-1) and the palette maps it to RGB. Indices 254/255 are
// reserved for the frontend's own UI (white/black, set once at canvas creation),
// so a game may use at most 254 colours (all puzzles use far fewer).
extern "C" void frontend_load_colours(frontend *fe, midend *me) {
  int n = 0;
  float *fl = midend_colours(me, &n);
  if (n > 254) n = 254;   // clamp: never overwrite the reserved UI entries
  fe->ncolours = n;
  for (int i = 0; i < n; i++) {
    int R = (int)(fl[i*3+0]*255.0f + 0.5f), G = (int)(fl[i*3+1]*255.0f + 0.5f),
        B = (int)(fl[i*3+2]*255.0f + 0.5f);
    if (R > 255) R = 255; if (G > 255) G = 255; if (B > 255) B = 255;
    fe->canvas->setPaletteColor(i, R, G, B);
  }
  free(fl);
}

// ---- primitive helpers ----

static inline frontend *FE(drawing *dr) { return GET_HANDLE_AS_TYPE(dr, frontend); }

// ---- drawing_api implementations ----

// All puzzle coords are translated by (offX,offY) so the board sits centered
// on the 240x135 canvas (set per game in startGame from midend_size).

static void d_rect(drawing *dr, int x, int y, int w, int h, int c) {
  frontend *fe = FE(dr);
  fe->canvas->fillRect(x + fe->offX, y + fe->offY, w, h, c);   // c = palette index
}

static void d_line(drawing *dr, int x1, int y1, int x2, int y2, int c) {
  frontend *fe = FE(dr);
  fe->canvas->drawLine(x1 + fe->offX, y1 + fe->offY, x2 + fe->offX, y2 + fe->offY, c);
}

static void d_circle(drawing *dr, int cx, int cy, int r, int fill, int outline) {
  frontend *fe = FE(dr); M5Canvas *cv = fe->canvas;
  cx += fe->offX; cy += fe->offY;
  if (fill >= 0) cv->fillCircle(cx, cy, r, fill);
  cv->drawCircle(cx, cy, r, outline);
}

static void d_poly(drawing *dr, const int *coords, int np, int fill, int outline) {
  frontend *fe = FE(dr); M5Canvas *cv = fe->canvas;
  int ox = fe->offX, oy = fe->offY;
  if (fill >= 0) {  // convex fan from vertex 0
    for (int i = 1; i + 1 < np; i++)
      cv->fillTriangle(coords[0]+ox, coords[1]+oy,
                       coords[2*i]+ox, coords[2*i+1]+oy,
                       coords[2*i+2]+ox, coords[2*i+3]+oy, fill);
  }
  for (int i = 0; i < np; i++) {  // outline, closed
    int j = (i + 1) % np;
    cv->drawLine(coords[2*i]+ox, coords[2*i+1]+oy, coords[2*j]+ox, coords[2*j+1]+oy,
                 outline);
  }
}

static void d_thick(drawing *dr, float th, float x1, float y1, float x2, float y2, int c) {
  (void)th; frontend *fe = FE(dr);
  fe->canvas->drawLine((int)x1 + fe->offX, (int)y1 + fe->offY,
                       (int)x2 + fe->offX, (int)y2 + fe->offY, c);
}

// ---- text ----
// textdatum constants live in lgfx::textdatum namespace, pulled into global scope
// via M5GFX's "using namespace lgfx::textdatum;" in M5GFX.h.
// Brief names (middle_center, baseline_left, etc.) match M5GFX v1 enum textdatum_t exactly.

static void d_text(drawing *dr, int x, int y, int fonttype, int fontsize,
                   int align, int colour, const char *text) {
  (void)fonttype;
  M5Canvas *cv = FE(dr)->canvas;
  int sz = (fontsize + 4) / 8; if (sz < 1) sz = 1;   // round-to-nearest font scale
  cv->setTextSize(sz);
  cv->setTextColor(colour);   // palette index
  textdatum_t datum;
  bool vc = (align & ALIGN_VCENTRE) != 0;
  if (align & ALIGN_HCENTRE)      datum = vc ? middle_center  : baseline_center;
  else if (align & ALIGN_HRIGHT)  datum = vc ? middle_right   : baseline_right;
  else                            datum = vc ? middle_left    : baseline_left;
  cv->setTextDatum(datum);
  cv->drawString(text, x + FE(dr)->offX, y + FE(dr)->offY);
}

// ---- clip / update / start / end / status ----

static void d_clip(drawing *dr, int x, int y, int w, int h) {
  FE(dr)->canvas->setClipRect(x + FE(dr)->offX, y + FE(dr)->offY, w, h);
}
static void d_unclip(drawing *dr) { FE(dr)->canvas->clearClipRect(); }
static void d_start(drawing *dr) {
  // Begin a fresh frame with no clip. A draw cycle must not inherit a clip rect
  // left set by the previous one (e.g. a game-over frame), or the next game
  // redraws into a stale region and looks frozen. (Host stubs clip to no-ops,
  // so this class of bug is only reproducible on device.)
  FE(dr)->canvas->clearClipRect();
}
static void d_end(drawing *dr) {
  frontend *fe = FE(dr);
  // Status bar: only games that declare wants_statusbar reserve the bottom strip
  // (sizeAndCenter keeps the board above y=124 for them). Paint it INTO the canvas
  // so the whole frame pushes atomically in one pushSprite -- drawing straight to
  // the panel after the push tears (two unsynchronized LCD writes per redraw show
  // as a flickering diagonal seam). Clear the strip every frame so old text can't
  // linger when the game's status string goes empty.
  if (fe->statusbar) {
    M5Canvas *cv = fe->canvas;
    cv->fillRect(0, 124, 240, 11, UI_BLACK);   // reserved strip
    if (fe->status[0]) {
      cv->setTextSize(1); cv->setTextColor(UI_WHITE, UI_BLACK);
      cv->setTextDatum(bottom_left);
      cv->drawString(fe->status, 2, 134);
    }
  }
  fe->canvas->pushSprite(&M5.Display, 0, 0);
}
static void d_update(drawing *dr, int, int, int, int) { (void)dr; }
static void d_status(drawing *dr, const char *t) {
  strncpy(FE(dr)->status, t ? t : "", sizeof(FE(dr)->status) - 1);
  FE(dr)->status[sizeof(FE(dr)->status) - 1] = '\0';
}

// ---- blitter ----

struct blitter { int w, h, x, y; uint8_t *buf; };   // 8bpp palette indices

static blitter *bl_new(drawing *dr, int w, int h) {
  (void)dr;
  blitter *bl = (blitter *)malloc(sizeof(blitter));
  if (!bl) fatal("bl_new: out of memory");
  bl->w = w; bl->h = h; bl->x = bl->y = -1;
  bl->buf = (uint8_t *)malloc(sizeof(uint8_t) * w * h);
  if (!bl->buf) fatal("bl_new: out of memory");
  return bl;
}
static void bl_free(drawing *dr, blitter *bl) {
  (void)dr;
  free(bl->buf); free(bl);
}
static void bl_save(drawing *dr, blitter *bl, int x, int y) {
  bl->x = x; bl->y = y;
  FE(dr)->canvas->readRect(x + FE(dr)->offX, y + FE(dr)->offY, bl->w, bl->h, bl->buf);
}
static void bl_load(drawing *dr, blitter *bl, int x, int y) {
  if (x == BLITTER_FROMSAVED) x = bl->x;
  if (y == BLITTER_FROMSAVED) y = bl->y;
  FE(dr)->canvas->pushImage(x + FE(dr)->offX, y + FE(dr)->offY, bl->w, bl->h, bl->buf);
}

// ---- stubs for unused slots ----

static void d_linewidth(drawing*, float) {}
static void d_linedotted(drawing*, bool) {}
static char *d_textfallback(drawing*, const char *const *s, int n) {
  char *r = s && n > 0 && s[0] ? strdup(s[0]) : strdup("");
  if (!r) fatal("d_textfallback: out of memory");
  return r;
}

// ---- drawing_api table ----
// Field order matches struct drawing_api in upstream/puzzles.h (26 members):
//   version, draw_text, draw_rect, draw_line, draw_polygon, draw_circle,
//   draw_update, clip, unclip, start_draw, end_draw, status_bar,
//   blitter_new, blitter_free, blitter_save, blitter_load,
//   begin_doc, begin_page, begin_puzzle, end_puzzle, end_page, end_doc,
//   line_width, line_dotted, text_fallback, draw_thick_line

extern "C" const drawing_api cardputer_drawing_api = {
  1,               // version
  d_text,          // draw_text
  d_rect,          // draw_rect
  d_line,          // draw_line
  d_poly,          // draw_polygon
  d_circle,        // draw_circle
  d_update,        // draw_update
  d_clip,          // clip
  d_unclip,        // unclip
  d_start,         // start_draw
  d_end,           // end_draw
  d_status,        // status_bar
  bl_new,          // blitter_new
  bl_free,         // blitter_free
  bl_save,         // blitter_save
  bl_load,         // blitter_load
  nullptr,         // begin_doc
  nullptr,         // begin_page
  nullptr,         // begin_puzzle
  nullptr,         // end_puzzle
  nullptr,         // end_page
  nullptr,         // end_doc
  d_linewidth,     // line_width
  d_linedotted,    // line_dotted
  d_textfallback,  // text_fallback
  d_thick,         // draw_thick_line
};

// ---- required frontend functions (C linkage; called by core .c) ----

// Crash-report context: which game + game-id was active when fatal() fires.
// Set by startGame before/after generation so the crash screen is a complete
// bug report even when the midend dies mid-generation.
static char g_crashGame[24] = "-";
static char g_crashId[48]   = "-";
extern "C" void frontend_set_crash(const char *game, const char *params) {
  strncpy(g_crashGame, game ? game : "-", sizeof g_crashGame - 1);
  g_crashGame[sizeof g_crashGame - 1] = '\0';
  strncpy(g_crashId, params ? params : "default", sizeof g_crashId - 1);
  g_crashId[sizeof g_crashId - 1] = '\0';
}

extern "C" void fatal(const char *fmt, ...) {
  Serial.print("FATAL: ");
  va_list ap; va_start(ap, fmt);
  char buf[160]; vsnprintf(buf, sizeof buf, fmt, ap); va_end(ap);
  Serial.printf("%s  [%s %s %s]\n", buf, FW_VERSION, g_crashGame, g_crashId);
  // Don't brick: show the error (with the game + game-id + version so a tester's
  // screenshot is a full bug report) and reboot back to the chooser.
  auto &d = M5.Display;
  d.fillScreen(TFT_BLACK); d.setTextSize(1); d.setTextDatum(middle_center);
  d.setTextColor(TFT_RED, TFT_BLACK);      d.drawString(buf, 120, 46);
  d.setTextColor(TFT_CYAN, TFT_BLACK);     d.drawString(g_crashGame, 120, 64);
  d.drawString(g_crashId, 120, 78);
  d.setTextColor(d.color565(0x67,0x88,0x99), TFT_BLACK);
  d.drawString(FW_VERSION "  -  report at the repo issues", 120, 96);
  d.setTextColor(TFT_RED, TFT_BLACK);      d.drawString("restarting...", 120, 116);
  delay(4000);
  ESP.restart();
}
extern "C" void frontend_default_colour(frontend *fe, float *output) {
  (void)fe;
  output[0] = output[1] = output[2] = 0.85f;
}
extern "C" void get_random_seed(void **randseed, int *randseedsize) {
  uint32_t *s = (uint32_t *)malloc(2 * sizeof(uint32_t));  // midend_new calls sfree() on this
  if (!s) fatal("get_random_seed: out of memory");
  s[0] = esp_random(); s[1] = esp_random();
  *randseed = s; *randseedsize = 2 * (int)sizeof(uint32_t);
}
extern "C" void activate_timer(frontend *fe)   { fe->timer_active = true; }
extern "C" void deactivate_timer(frontend *fe) { fe->timer_active = false; }
