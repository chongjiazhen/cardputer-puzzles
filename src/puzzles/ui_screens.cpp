#include <M5Cardputer.h>
#undef PI   // puzzles.h (via params_ui.h) redefines PI with more precision
#include <cstdio>
#include <cstring>
#include <climits>
#include "params_ui.h"
#include "picker.h"
#include "rules_map.h"

namespace puz {

static UiCtx g_ui;
void uiBind(const UiCtx& c) { g_ui = c; }

// ---------------- rules viewer ----------------
static char* g_rulesText = nullptr;   // owned by caller (midend_get_config returns malloc'ed)
static int   g_rulesLine = 0;         // top line visible at scroll origin
static char kLines[18][64];           // mutable buffers, one per wrapped line
static int   g_linesN = 0;            // total wrapped lines
static const int RULES_CHARS_PER_LINE = 18;  // size-2 font ~12px wide => ~18 chars in 224px usable width

static void freeRules() { memset(kLines, 0, sizeof kLines); g_linesN = 0; g_rulesLine = 0; if (g_rulesText) sfree(g_rulesText), g_rulesText = nullptr; }

// Wrap `src` into lines that fit on a single row.
static const int MAX_LINES = 18;

static void wrapRules(const char* src) {
  freeRules();
  g_rulesText = strdup(src);
  int n = 0;
  const char *p = g_rulesText;
  while (*p && n < MAX_LINES) {
    // Find end of this paragraph: double newline or EOF
    const char* paraEnd = p;
    const char* nn = strstr(p, "\n\n");
    if (nn) paraEnd = nn;
    else paraEnd = p + strlen(p);

    // Trim trailing whitespace from paragraph body
    while (paraEnd > p && (*(paraEnd-1) == ' ' || *(paraEnd-1) == '\t' || *(paraEnd-1) == '\n'))
      paraEnd--;
    size_t plen = paraEnd - p;
    if (plen == 0) {
      p = (nn) ? nn + 2 : paraEnd;  // skip blank paragraph separator
      continue;
    }

    // Word-wrap this paragraph within RULES_CHARS_PER_LINE chars.
    const char* q = p;
    size_t qlen = plen;
    while (qlen > 0 && n < MAX_LINES) {
      int limit = (qlen < (size_t)RULES_CHARS_PER_LINE) ? (int)qlen : RULES_CHARS_PER_LINE;
      int cut = limit;
      if (cut < (int)qlen) {
        int space = -1;
        for (int i = 0; i < limit; i++) if (q[i] == ' ') space = i;
        if (space > 0) cut = space;
      }
      int copyLen = (cut > 63) ? 63 : cut;
      memcpy(kLines[n], q, copyLen);
      kLines[n][copyLen] = '\0';
      n++;
      q += cut;
      qlen -= cut;
      while (qlen > 0 && (*q == ' ' || *q == '\t')) { q++; qlen--; }
    }
    // Advance past consumed paragraph + separator
    p = (nn) ? nn + 2 : paraEnd;
    while (*p == '\n' || *p == ' ' || *p == '\t') p++;
  }
  g_linesN = n;
}

static const char* lookup_rule(const char* topic) {
    for (int i = 0; rules_map[i].topic != NULL; ++i) {
        if (strcmp(rules_map[i].topic, topic) == 0)
            return rules_map[i].text;
    }
    return NULL;
}

static void drawRules() {
  auto &d = M5.Display;
  d.fillScreen(TFT_BLACK);
  d.setTextSize(1); d.setTextColor(TFT_CYAN, TFT_BLACK);
  d.setTextDatum(top_left); d.drawString("Rules", 4, 2);
  d.drawFastHLine(0, 12, 240, d.color565(0x24, 0x40, 0x55));
  d.setTextColor(TFT_WHITE, TFT_BLACK);
  d.setTextSize(2);

  int y = 20;
  for (int i = g_rulesLine; i < g_linesN && y <= 130; i++, y += 18)
    d.drawString(kLines[i], 8, y);

  d.setTextSize(1);
  d.setTextColor(d.color565(0x67, 0x88, 0x99), TFT_BLACK);
  d.setTextDatum(bottom_left); d.drawString("` back", 4, 133);
}

void openRules() {      
  const char *topic = midend_which_game(g_ui.me)->htmlhelp_topic;
  const char *rule = lookup_rule(topic);
  const char *fallback = "No detailed rules available on this device.\nSee the online documentation for game-specific instructions.";
  const char *finalDesc = rule ? rule : fallback;
  wrapRules(finalDesc);
  g_ui.toRules();
  drawRules();
}

void ruleKey(InputEvent ev) {
  if (g_linesN == 0) { if (ev.kind == Ev::BackToChooser) { freeRules(); g_ui.resume(); } return; }
  switch (ev.kind) {
    case Ev::Up:   g_rulesLine = (g_rulesLine + g_linesN - 1) % g_linesN; drawRules(); break;
    case Ev::Down: g_rulesLine = (g_rulesLine + 1) % g_linesN;          drawRules(); break;
    case Ev::BackToChooser: freeRules(); g_ui.resume(); break;
    default: break;
  }
}

// ---------------- command menu ----------------
static const char* kCmdItems[]   = {"Size / Type","New game","Restart","Solve","Undo","Redo","Pointer","Rules"};
static const char* kCmdSuffix[]  = {"",           "Ctrl+N",  "Ctrl+R", "",     "Ctrl+Z","Ctrl+Y","Ctrl+P"," Tab "};
static const int   kCmdN = 8;
static int g_cmdSel = 0;

static void drawCommand() { drawPicker(kCmdItems, kCmdN, g_cmdSel, "Menu", "Tab", kCmdSuffix, 5); }
void openCommand() { g_cmdSel = 0; drawCommand(); }

void commandKey(InputEvent ev) {
  switch (ev.kind) {
    case Ev::Up:   g_cmdSel = (g_cmdSel + kCmdN - 1) % kCmdN; drawCommand(); break;
    case Ev::Down: g_cmdSel = (g_cmdSel + 1) % kCmdN;         drawCommand(); break;
    case Ev::BackToChooser: case Ev::CommandMenu: g_ui.resume(); break;
    case Ev::Select:
      switch (g_cmdSel) {
        case 0: g_ui.toType(); openTypeMenu(); break;
        case 1: midend_new_game(g_ui.me); g_ui.reloadResume(); break;
        case 2: midend_restart_game(g_ui.me); g_ui.resume(); break;
        case 3: midend_solve(g_ui.me); g_ui.resume(); break;
        case 4: midend_process_key(g_ui.me, 0, 0, UI_UNDO); g_ui.resume(); break;
        case 5: midend_process_key(g_ui.me, 0, 0, UI_REDO); g_ui.resume(); break;
        case 6: g_ui.togglePointer(); g_ui.resume(); break;
        case 7: g_ui.resume(); openRules(); break;
      }
      break;
    default: break;
  }
}

// ---------------- type (preset) menu ----------------
static PresetEntry g_presets[64];
static const char* g_titles[64];
static int g_presetN = 0, g_typeSel = 0;
static char g_typePos[12];

static void drawType() {
  for (int i = 0; i < g_presetN; i++) g_titles[i] = g_presets[i].title;
  snprintf(g_typePos, sizeof g_typePos, "%d/%d", g_typeSel + 1, g_presetN);
  drawPicker(g_titles, g_presetN, g_typeSel, "Type", g_typePos, nullptr, 5);
}

void openTypeMenu() {
  g_presetN = flattenPresets(g_ui.me, g_presets, 64);
  g_typeSel = 0;
  drawType();
}

void typeKey(InputEvent ev) {
  switch (ev.kind) {
    case Ev::Up:   g_typeSel = (g_typeSel + g_presetN - 1) % g_presetN; drawType(); break;
    case Ev::Down: g_typeSel = (g_typeSel + 1) % g_presetN;             drawType(); break;
    case Ev::BackToChooser: g_ui.resume(); break;
    case Ev::Select:
      if (g_presets[g_typeSel].id < 0) { g_ui.toConfig(); openConfig(); }   // "Custom..."
      else { applyPreset(g_ui.me, g_presets[g_typeSel].id); g_ui.reloadResume(); }
      break;
    default: break;
  }
}

// ---------------- custom config editor ----------------
static config_item* g_cfg = nullptr;
static char* g_cfgTitle = nullptr;
static int g_cfgN = 0, g_cfgSel = 0;
static const char* g_cfgErr = nullptr;

static int cfgCount(config_item* c) { int n = 0; while (c[n].type != C_END) n++; return n; }

// Extract the selected option name from a C_CHOICES ":a:b:c" string.
static void choiceName(const config_item* ci, char* out, int cap) {
  const char* cn = ci->u.choices.choicenames;
  char sep = cn[0];
  int want = ci->u.choices.selected, idx = 0;
  const char* start = cn + 1;
  const char* p = cn + 1;
  for (; *p; p++) {
    if (*p == sep) { if (idx == want) break; idx++; start = p + 1; }
  }
  int len = (int)(p - start); if (len > cap - 1) len = cap - 1;
  memcpy(out, start, len); out[len] = '\0';
}

static int choiceCount(const config_item* ci) {
  const char* cn = ci->u.choices.choicenames; char sep = cn[0]; int n = 0;
  for (const char* p = cn; *p; p++) if (*p == sep) n++;
  return n;
}

static void drawConfig() {
  auto &d = M5.Display;
  d.fillScreen(TFT_BLACK);
  d.setTextSize(1); d.setTextColor(TFT_CYAN, TFT_BLACK);
  d.setTextDatum(top_left); d.drawString("Custom", 4, 2);
  d.drawFastHLine(0, 12, 240, d.color565(0x24, 0x40, 0x55));
  for (int i = 0; i < g_cfgN; i++) {
    int y = 24 + i * 16;
    if (i == g_cfgSel) { d.fillRoundRect(2, y - 7, 236, 15, 2, TFT_WHITE); d.setTextColor(TFT_BLACK, TFT_WHITE); }
    else d.setTextColor(TFT_WHITE, TFT_BLACK);
    d.setTextDatum(middle_left); d.drawString(g_cfg[i].name, 8, y);
    char val[40];
    switch (g_cfg[i].type) {
      case C_STRING:  snprintf(val, sizeof val, "%s", g_cfg[i].u.string.sval); break;
      case C_BOOLEAN: snprintf(val, sizeof val, "%s", g_cfg[i].u.boolean.bval ? "on" : "off"); break;
      case C_CHOICES: choiceName(&g_cfg[i], val, sizeof val); break;
      default: val[0] = '\0';
    }
    d.setTextDatum(middle_right); d.drawString(val, 232, y);
  }
  d.setTextColor(d.color565(0x67, 0x88, 0x99), TFT_BLACK);
  d.setTextDatum(bottom_left);
  d.drawString(g_cfgErr ? g_cfgErr : ";/. field  type edit  Enter apply  ` cancel", 4, 133);
}

void openConfig() {
  if (g_cfg) configFree(g_cfg);
  free(g_cfgTitle); g_cfgTitle = nullptr;   // title is caller-owned; free prior fetch
  g_cfgErr = nullptr; g_cfgSel = 0;
  g_cfg = configBegin(g_ui.me, &g_cfgTitle);
  g_cfgN = cfgCount(g_cfg);
  drawConfig();
}

void configKey(InputEvent ev) {
  config_item* ci = &g_cfg[g_cfgSel];
  switch (ev.kind) {
    case Ev::Up:   g_cfgSel = (g_cfgSel + g_cfgN - 1) % g_cfgN; g_cfgErr = nullptr; drawConfig(); break;
    case Ev::Down: g_cfgSel = (g_cfgSel + 1) % g_cfgN;          g_cfgErr = nullptr; drawConfig(); break;
    case Ev::Left: case Ev::Right: {
      int dir = (ev.kind == Ev::Right) ? 1 : -1;
      if (ci->type == C_BOOLEAN) ci->u.boolean.bval = !ci->u.boolean.bval;
      else if (ci->type == C_CHOICES) {
        int n = choiceCount(ci);
        ci->u.choices.selected = (ci->u.choices.selected + dir + n) % n;
      }
      drawConfig(); break;
    }
    case Ev::Select2:
      if (ci->type == C_BOOLEAN) { ci->u.boolean.bval = !ci->u.boolean.bval; drawConfig(); }
      break;
    case Ev::Char:
      if (ci->type == C_STRING) {
        if (ev.ch == '\b') configStringEdit(ci, 0, true);
        else if (ev.ch >= ' ') configStringEdit(ci, ev.ch, false);
        drawConfig();
      }
      break;
    case Ev::Select: {   // apply all
      const char* err = configApply(g_ui.me, g_cfg);   // sets params only — must regenerate
      if (err) { g_cfgErr = err; drawConfig(); }
      else {
        midend_new_game(g_ui.me);   // generate a board at the new params (else stale grid
                                    // draws into a differently-sized background)
        configFree(g_cfg); g_cfg = nullptr; free(g_cfgTitle); g_cfgTitle = nullptr;
        g_ui.reloadResume();
      }
      break;
    }
    case Ev::BackToChooser:   // cancel
      configFree(g_cfg); g_cfg = nullptr; free(g_cfgTitle); g_cfgTitle = nullptr; g_ui.resume(); break;
    default: break;
  }
}

}  // namespace puz
