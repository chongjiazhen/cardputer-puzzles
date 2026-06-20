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

}  // namespace puz
