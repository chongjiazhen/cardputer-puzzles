#pragma once
// presets.h — per-game starting params tuned for the 240x135 Cardputer screen.
//
// Upstream defaults are desktop-sized; a few games pack too many cells to stay
// legible at 135 px tall. startGame() applies these via midend_game_id() before
// midend_new_game(). Games absent from the table keep their upstream default
// (already screen-friendly — verified by the host test's reported grid sizes).
//
// Each string is host-verified to both parse AND generate (startgame_test runs
// midend_new_game after applying it, so a bad format or an unsolvable-at-this-size
// config fails the test). The exact values are legibility starting points —
// bump them on hardware once you can eyeball the real font.
//
// To tune a game: add/edit a row here. Name must match game.name exactly
// (e.g. "Light Up", "Same Game", "Train Tracks"). params format is the game's
// "Game ID" string (see each backend's decode_params).
#include <string.h>

struct GamePreset { const char *name; const char *params; };

static const struct GamePreset kGamePresets[] = {
  { "Mines",   "8x8n10"  },  // default 9x9n10 — fewer cells so mine-count digits stay readable
  { "Filling", "9x7"     },  // default 13x9 — larger digit cells
  { "Pattern", "10x10"   },  // default 15x15 — nonogram clue numbers stay legible
  { "Flood",   "10x10c4" },  // default 12x12c6 — fewer/larger colour blocks
};
static const int kGamePresetCount =
  (int)(sizeof(kGamePresets) / sizeof(kGamePresets[0]));

// Return the params string for game `name`, or nullptr to keep the upstream default.
static inline const char *presetFor(const char *name) {
  for (int i = 0; i < kGamePresetCount; i++)
    if (strcmp(name, kGamePresets[i].name) == 0)
      return kGamePresets[i].params;
  return nullptr;
}
