#include "frontend.h"

// BLITTER_FROMSAVED: sentinel not defined in upstream puzzles.h; define locally.
// Callers (game backends) pass this to reuse the saved x/y. Net does not use blitters.
#ifndef BLITTER_FROMSAVED
#define BLITTER_FROMSAVED (-1)
#endif

// ---- colour conversion ----

static inline uint16_t rgb565(float r, float g, float b) {
  int R = (int)(r * 31.0f + 0.5f), G = (int)(g * 63.0f + 0.5f), B = (int)(b * 31.0f + 0.5f);
  if (R > 31) R = 31; if (G > 63) G = 63; if (B > 31) B = 31;
  if (R < 0) R = 0; if (G < 0) G = 0; if (B < 0) B = 0;
  return (uint16_t)((R << 11) | (G << 5) | B);
}

extern "C" void frontend_load_colours(frontend *fe, midend *me) {
  int n = 0;
  float *fl = midend_colours(me, &n);
  free(fe->colours);
  fe->colours = (uint16_t *)malloc(sizeof(uint16_t) * n);
  fe->ncolours = n;
  for (int i = 0; i < n; i++)
    fe->colours[i] = rgb565(fl[i*3+0], fl[i*3+1], fl[i*3+2]);
  free(fl);
}

// ---- primitive helpers ----

static inline frontend *FE(drawing *dr) { return GET_HANDLE_AS_TYPE(dr, frontend); }

// ---- drawing_api implementations ----

// All puzzle coords are translated by (offX,offY) so the board sits centered
// on the 240x135 canvas (set per game in startGame from midend_size).

static void d_rect(drawing *dr, int x, int y, int w, int h, int c) {
  frontend *fe = FE(dr);
  fe->canvas->fillRect(x + fe->offX, y + fe->offY, w, h, fe->colours[c]);
}

static void d_line(drawing *dr, int x1, int y1, int x2, int y2, int c) {
  frontend *fe = FE(dr);
  fe->canvas->drawLine(x1 + fe->offX, y1 + fe->offY, x2 + fe->offX, y2 + fe->offY, fe->colours[c]);
}

static void d_circle(drawing *dr, int cx, int cy, int r, int fill, int outline) {
  frontend *fe = FE(dr); M5Canvas *cv = fe->canvas;
  cx += fe->offX; cy += fe->offY;
  if (fill >= 0) cv->fillCircle(cx, cy, r, fe->colours[fill]);
  cv->drawCircle(cx, cy, r, fe->colours[outline]);
}

static void d_poly(drawing *dr, const int *coords, int np, int fill, int outline) {
  frontend *fe = FE(dr); M5Canvas *cv = fe->canvas;
  int ox = fe->offX, oy = fe->offY;
  if (fill >= 0) {  // convex fan from vertex 0
    for (int i = 1; i + 1 < np; i++)
      cv->fillTriangle(coords[0]+ox, coords[1]+oy,
                       coords[2*i]+ox, coords[2*i+1]+oy,
                       coords[2*i+2]+ox, coords[2*i+3]+oy, fe->colours[fill]);
  }
  for (int i = 0; i < np; i++) {  // outline, closed
    int j = (i + 1) % np;
    cv->drawLine(coords[2*i]+ox, coords[2*i+1]+oy, coords[2*j]+ox, coords[2*j+1]+oy,
                 fe->colours[outline]);
  }
}

static void d_thick(drawing *dr, float th, float x1, float y1, float x2, float y2, int c) {
  (void)th; frontend *fe = FE(dr);
  fe->canvas->drawLine((int)x1 + fe->offX, (int)y1 + fe->offY,
                       (int)x2 + fe->offX, (int)y2 + fe->offY, fe->colours[c]);
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
  cv->setTextColor(FE(dr)->colours[colour]);
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
  FE(dr)->canvas->pushSprite(&M5.Display, 0, 0);
}
static void d_update(drawing *dr, int, int, int, int) { (void)dr; }
static void d_status(drawing *dr, const char *t) {
  strncpy(FE(dr)->status, t ? t : "", sizeof(FE(dr)->status) - 1);
  FE(dr)->status[sizeof(FE(dr)->status) - 1] = '\0';
}

// ---- blitter ----

struct blitter { int w, h, x, y; uint16_t *buf; };

static blitter *bl_new(drawing *dr, int w, int h) {
  (void)dr;
  blitter *bl = (blitter *)malloc(sizeof(blitter));
  bl->w = w; bl->h = h; bl->x = bl->y = -1;
  bl->buf = (uint16_t *)malloc(sizeof(uint16_t) * w * h);
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
  return s && n > 0 && s[0] ? strdup(s[0]) : strdup("");
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

extern "C" void fatal(const char *fmt, ...) {
  Serial.print("FATAL: ");
  va_list ap; va_start(ap, fmt);
  char buf[160]; vsnprintf(buf, sizeof buf, fmt, ap); va_end(ap);
  Serial.println(buf);
  // Don't brick: show the error and reboot back to the chooser. (A game that
  // exhausts SRAM mid-generation leaves the midend unrecoverable, so restart.)
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(TFT_RED, TFT_BLACK);
  M5.Display.setTextDatum(middle_center);
  M5.Display.drawString(buf, 120, 58);
  M5.Display.drawString("restarting...", 120, 76);
  delay(2500);
  ESP.restart();
}
extern "C" void frontend_default_colour(frontend *fe, float *output) {
  (void)fe;
  output[0] = output[1] = output[2] = 0.85f;
}
extern "C" void get_random_seed(void **randseed, int *randseedsize) {
  uint32_t *s = (uint32_t *)malloc(2 * sizeof(uint32_t));  // midend_new calls sfree() on this
  s[0] = esp_random(); s[1] = esp_random();
  *randseed = s; *randseedsize = 2 * (int)sizeof(uint32_t);
}
extern "C" void activate_timer(frontend *fe)   { fe->timer_active = true; }
extern "C" void deactivate_timer(frontend *fe) { fe->timer_active = false; }
