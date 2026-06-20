#pragma once
extern "C" {
#include "puzzles.h"
}

namespace puz {

struct PresetEntry { const char* title; int id; };   // id == -1 => "Custom..."

// Flatten the midend preset tree (depth-first leaves) + append "Custom...".
// Returns number of entries written (<= maxOut).
int flattenPresets(midend* me, PresetEntry* out, int maxOut);

// Apply a real preset by id (set_params + new_game). id<0 (Custom) returns false.
bool applyPreset(midend* me, int id);

// --- Custom config editor (CFG_SETTINGS) ---
config_item* configBegin(midend* me, char** title);      // = midend_get_config(CFG_SETTINGS)
const char*  configApply(midend* me, config_item* cfg);  // = midend_set_config; NULL ok, else err
void         configFree(config_item* cfg);               // = free_cfg
void         configStringEdit(config_item* ci, char c, bool backspace);  // grow/shrink C_STRING sval

}  // namespace puz
