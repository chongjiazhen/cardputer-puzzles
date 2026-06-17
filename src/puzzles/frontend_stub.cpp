#include <Arduino.h>
extern "C" {
#undef PI   // puzzles.h redefines PI with more precision; suppress redefinition warning
#include "puzzles.h"
}

// ---- required frontend functions (C linkage; called by core .c) ----
extern "C" void fatal(const char *fmt, ...) {
  Serial.print("FATAL: ");
  va_list ap; va_start(ap, fmt);
  char buf[160]; vsnprintf(buf, sizeof buf, fmt, ap); va_end(ap);
  Serial.println(buf);
  for (;;) delay(1000);
}
extern "C" void frontend_default_colour(frontend *fe, float *output) {
  output[0] = output[1] = output[2] = 0.85f;
}
extern "C" void get_random_seed(void **randseed, int *randseedsize) {
  static uint32_t s[2];
  s[0] = esp_random(); s[1] = esp_random();
  *randseed = s; *randseedsize = sizeof s;
}
extern "C" void activate_timer(frontend *fe) {}
extern "C" void deactivate_timer(frontend *fe) {}

// ---- no-op drawing api ----
// Slot order must match struct drawing_api in upstream/puzzles.h exactly (26 members):
//   version, draw_text, draw_rect, draw_line, draw_polygon, draw_circle,
//   draw_update, clip, unclip, start_draw, end_draw, status_bar,
//   blitter_new, blitter_free, blitter_save, blitter_load,
//   begin_doc, begin_page, begin_puzzle, end_puzzle, end_page, end_doc,
//   line_width, line_dotted, text_fallback, draw_thick_line
static void d_text(drawing*, int, int, int, int, int, int, const char*) {}
static void d_rect(drawing*, int, int, int, int, int) {}
static void d_line(drawing*, int, int, int, int, int) {}
static void d_poly(drawing*, const int*, int, int, int) {}
static void d_circle(drawing*, int, int, int, int, int) {}
static void d_update(drawing*, int, int, int, int) {}
static void d_clip(drawing*, int, int, int, int) {}
static void d_unclip(drawing*) {}
static void d_start(drawing*) {}
static void d_end(drawing*) {}
static void d_status(drawing*, const char*) {}
static blitter *bl_new(drawing*, int, int) { return nullptr; }
static void bl_free(drawing*, blitter*) {}
static void bl_save(drawing*, blitter*, int, int) {}
static void bl_load(drawing*, blitter*, int, int) {}
static void d_linewidth(drawing*, float) {}
static void d_linedotted(drawing*, bool) {}
static char *d_textfallback(drawing*, const char *const *strings, int) {
  return strings && strings[0] ? strdup(strings[0]) : strdup("");
}
static void d_thick(drawing*, float, float, float, float, float, int) {}

extern "C" const drawing_api stub_drawing_api = {
  1,           // version
  d_text,      // draw_text
  d_rect,      // draw_rect
  d_line,      // draw_line
  d_poly,      // draw_polygon
  d_circle,    // draw_circle
  d_update,    // draw_update
  d_clip,      // clip
  d_unclip,    // unclip
  d_start,     // start_draw
  d_end,       // end_draw
  d_status,    // status_bar
  bl_new,      // blitter_new
  bl_free,     // blitter_free
  bl_save,     // blitter_save
  bl_load,     // blitter_load
  nullptr,     // begin_doc
  nullptr,     // begin_page
  nullptr,     // begin_puzzle
  nullptr,     // end_puzzle
  nullptr,     // end_page
  nullptr,     // end_doc
  d_linewidth, // line_width
  d_linedotted,// line_dotted
  d_textfallback, // text_fallback
  d_thick,     // draw_thick_line
};
