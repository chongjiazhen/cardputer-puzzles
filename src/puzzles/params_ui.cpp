#include "params_ui.h"

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

}  // namespace puz
