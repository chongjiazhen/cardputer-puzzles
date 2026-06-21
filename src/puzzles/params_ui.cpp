#include "params_ui.h"
#include <cstring>
#include <cstdlib>

namespace puz {

static void walk(struct preset_menu* m, PresetEntry* out, int maxOut, int* n) {
  if (!m) return;
  for (int i = 0; i < m->n_entries; i++) {
    struct preset_menu_entry* e = &m->entries[i];
    if (e->submenu)                       walk(e->submenu, out, maxOut, n);
    else if (e->params && *n < maxOut) { out[*n].title = e->title; out[*n].id = e->id; (*n)++; }
  }
}

int flattenPresets(midend* me, PresetEntry* out, int maxOut) {
  int idlimit = 0;
  struct preset_menu* pm = midend_get_presets(me, &idlimit);
  int n = 0;
  walk(pm, out, maxOut, &n);
  if (n < maxOut) { out[n].title = "Custom..."; out[n].id = -1; n++; }
  return n;
}

bool applyPreset(midend* me, int id) {
  if (id < 0) return false;
  int idlimit = 0;
  struct preset_menu* pm = midend_get_presets(me, &idlimit);
  game_params* p = preset_menu_lookup_by_id(pm, id);
  if (!p) return false;
  midend_set_params(me, p);
  midend_new_game(me);
  return true;
}

config_item* configBegin(midend* me, char** title) {
  return midend_get_config(me, CFG_SETTINGS, title);
}

const char* configApply(midend* me, config_item* cfg) {
  return midend_set_config(me, CFG_SETTINGS, cfg);
}

void configFree(config_item* cfg) { free_cfg(cfg); }

void configStringEdit(config_item* ci, char c, bool backspace) {
  if (ci->type != C_STRING) return;
  char* s = ci->u.string.sval;              // dynamically allocated, non-NULL
  size_t len = s ? strlen(s) : 0;
  if (backspace) {
    if (len) s[len - 1] = '\0';
  } else {
    char* ns = (char*)realloc(s, len + 2);  // midend smalloc == malloc, realloc-safe
    if (!ns) return;                        // OOM: keep existing string, drop keystroke
    ns[len] = c; ns[len + 1] = '\0';
    ci->u.string.sval = ns;
  }
}

}  // namespace puz
