# Params UI + Picker Widget — Design Spec

**Date:** 2026-06-20
**Status:** Approved design, pre-plan
**Scope:** Spec 1 of 2. This spec = on-device params control (presets + custom config) + a reusable picker widget + a collision-free keymap + a Tab command menu. **Out of scope (Spec 2):** favorites/star, global device settings, NVS persistence. The picker built here is generic so Spec 2 reuses it with zero rework.

## Goal

Let the player change each puzzle's size/difficulty on-device — quick preset cycling, a Type preset menu, and a Custom per-field editor — the canonical Simon Tatham "Type / Custom" model, on a 240×135 screen. Replace the flat game menu with a comfortable picker, and fix a systemic keymap collision where frontend letter-shortcuts steal game input.

## Background / constraints

- Screen 240×135, M5GFX fonts: size 1 = 6×8 px, size 2 = 12×16 px.
- midend already exposes everything needed (verified in `upstream/puzzles.h`):
  - `midend_get_presets(me, &id_limit)` → `struct preset_menu*` (tree; leaves carry `title`, `game_params*`, `id`).
  - `midend_which_preset(me)` → current preset id, or negative if custom.
  - `preset_menu_lookup_by_id(menu, id)` → `game_params*`.
  - `midend_set_params(me, params)` then `midend_new_game(me)` to apply.
  - `midend_get_config(me, CFG_SETTINGS, &title)` → `config_item*` array (C_END-terminated).
  - `midend_set_config(me, CFG_SETTINGS, cfg)` → `const char*` error or NULL; on NULL caller runs `midend_new_game`.
  - `free_cfg(cfg)` to release a config_item array.
  - `config_item` types: `C_STRING` (`u.string.sval`), `C_CHOICES` (`u.choices.choicenames` ":a:b:c", `u.choices.selected`), `C_BOOLEAN` (`u.boolean.bval`).
- Cardputer ADV keyboard matrix (from `M5Cardputer/.../Keyboard.h`): printable chars in `KeysState.word`; `Tab`/`Enter`/`Del`/`Ctrl`/`Fn`/`Shift`/`Opt`/`Alt` are separate bool flags. `[ ] \ ` ` ' - =` are real unshifted keys; `^` is Shift+6 (a real glyph — so hotkey hints spell out `Ctrl`, never `^`).

## Keymap collision (the bug this also fixes)

Audit of all 40 games: 16 use letter keys as input (e.g. solo a–p hex digits, undead g/v/z monsters, map l labels, twiddle a–d, net a/s/d/f/j). The current frontend steals `u r n s k l p` for shortcuts → direct collisions (`s`→Solve hits net/galaxies; `l`→click hits map; etc.). **Rule: every plain key belongs to the game.** Frontend commands move to non-letter keys + the Ctrl modifier + a Tab menu.

Verified free of game-input use: `[ ] \ ` ` ' Tab - =`. Only `slant` uses `/` (draws "/" lines) — clashes with cursor-right but slant stays fully playable via cursor+select. Accepted.

## Components / files

| File | Responsibility |
|---|---|
| `src/puzzles/picker.h` (new) | `drawPicker(items[], n, sel, title, posText)` — Hybrid-7 circular picker render. Pure; no state. Used by game chooser, Type menu, command menu. |
| `src/puzzles/params_ui.h/.cpp` (new) | Preset flattening, cycler, Type-menu + Custom-editor + command-menu state logic and rendering. Owns the midend preset/config calls. |
| `src/puzzles/menu.h` | Game chooser switches to `drawPicker`. |
| `src/puzzles/input.h` | Extend `Ev` enum (below). |
| `src/puzzles/keymap.h` | Map `(char, ctrl, tab)` → `Ev`. |
| `lib/cardputer_hw/cardputer_hw.{h,cpp}` | New modifier-aware key read (below). |
| `src/puzzles/main.cpp` | New states, route keys, call `params_ui`. |
| `src/puzzles/startgame_test.cpp` | Host tests for preset + config logic. |

## Input layer (modifier-aware)

`keysJustPressed()` today returns `vector<char>` (word + `\r` enter + `\b` del). It cannot express Ctrl or Tab. Replace with:

```cpp
struct KeyPress { char ch; bool ctrl; };          // ch: printable, or '\r' \b '\t' sentinels
std::vector<KeyPress> cardputer::keysJustPressedEx();
```

Emission from `KeysState`: each `word` char → `{c, status.ctrl}`; `status.enter` → `{'\r',false}`; `status.del` → `{'\b',false}`; `status.tab` → `{'\t',false}`. (Existing `keysJustPressed()` kept as a thin wrapper for any caller that only wants chars, or removed if unused.)

## Keymap (`Ev` enum + mapping)

New/changed `Ev`: `CommandMenu` (Tab), `BackToChooser` (`` ` ``), `Restart`. Keep `Up/Down/Left/Right/Select/Select2/Char/Undo/Redo/NewGame/Solve/ClickL/ClickR`. Drop `TogglePointer`/`Menu` key bindings (move into command menu / rename).

`eventForKey(KeyPress k)`:

| Input | Ev |
|---|---|
| `;` `.` `,` `/` | Up / Down / Left / Right |
| `\r` (Enter) | Select |
| Space | Select2 |
| `[` / `]` | ClickL / ClickR |
| Ctrl+`z` / Ctrl+`y` | Undo / Redo |
| Ctrl+`n` / Ctrl+`r` | NewGame / Restart |
| `\t` (Tab) | CommandMenu |
| `` ` `` | BackToChooser |
| other printable (`ch`, no ctrl) | Char(ch) → game |

Note: digits, letters, `\b` fall through as `Char` → forwarded to the game. Nothing stolen.

## State machine

States: `MENU` (game chooser), `PLAYING`, `TYPE_MENU`, `CONFIG_EDIT`, `COMMAND` (Tab menu), `HELP` (controls reference).

```
MENU ──Select──▶ PLAYING
MENU ──Tab──▶ HELP ──`──▶ MENU
PLAYING ──`──▶ MENU
PLAYING ──Tab──▶ COMMAND
COMMAND ──pick "Size/Type"──▶ TYPE_MENU
COMMAND ──pick New/Restart/Solve/Pointer──▶ (action) ──▶ PLAYING
COMMAND ──`/Tab──▶ PLAYING (close)
TYPE_MENU ──pick preset──▶ apply+regen ──▶ PLAYING
TYPE_MENU ──pick "Custom…"──▶ CONFIG_EDIT
TYPE_MENU ──`──▶ PLAYING
CONFIG_EDIT ──Enter(valid)──▶ apply+regen ──▶ PLAYING
CONFIG_EDIT ──`──▶ PLAYING (cancel)
```

In PLAYING, `Undo/Redo/NewGame/Restart` apply directly (also reachable via COMMAND). `MENU`, `COMMAND`, `TYPE_MENU`, `CONFIG_EDIT` use plain keys for their own UI (no game running → no collision).

## Picker widget (Hybrid-7)

`drawPicker(const char* items[], int n, int sel, const char* title, const char* pos)`:
- Header row: `title` left, `pos` right (e.g. `8/40`), 8 px, cyan, underline rule.
- Up to 7 rows centered on `sel`: the selected row centered, bold, inverted (12 px); up to 3 neighbors each side at 8 px with graded dim (`#9a9a9a`, `#5e5e5e`, `#3a3a3a`).
- **Circular**: neighbors wrap past list ends. `pos` counter is the place anchor.
- Optional right-aligned per-row suffix (used by command menu to show hotkeys `Ctrl+N`, and Type menu has none). Implement as an optional parallel `suffix[]` array (NULL = none).

Game chooser, Type menu, and command menu all call it. Chooser scroll math (current sel-centered window) is superseded by this. Chooser lists games **alphabetically by display name** (matching the Android port + upstream website); `gamelist[]` is already ordered this way.

## Preset cycling + Type menu

Flatten `midend_get_presets()` tree → ordered array of `{title, id}` leaves (depth-first; skip submenu title entries, keep leaves). Append a synthetic `"Custom…"` entry.

- **Cycle** (from command menu "Size/Type" is the menu path; there is no standalone cycle key after the collision fix — cycling happens by scrolling the Type wheel): N/A as a hotkey. (The earlier `t` quick-cycle is dropped — `t` collides with tracks. Scrolling the Type wheel is the cycle.)
- **Type menu**: `drawPicker` over the flattened preset titles + "Custom…". `;`/`.` scroll (circular), Enter applies: for a preset → `preset_menu_lookup_by_id(presets, id)` → `midend_set_params` → `midend_new_game` → reload colours/size/force_redraw → PLAYING. For "Custom…" → CONFIG_EDIT. `` ` `` cancels.

## Custom config editor (inline)

On entry: `cfg = midend_get_config(me, CFG_SETTINGS, &title)`. Render the field list (all fit; ≤ ~6 fields typical):
- Field row: name left, value right; selected field inverted.
- `;`/`.` move between fields (circular).
- Editing the selected field by type:
  - `C_STRING`: digits/letters append to `u.string.sval` (a growable copy); `\b` deletes last char. (Numeric fields are C_STRING holding digits.)
  - `C_CHOICES`: `,`/`/` decrement/increment `u.choices.selected` (clamped).
  - `C_BOOLEAN`: Space or `,`/`/` toggles `u.boolean.bval`.
- Enter → `err = midend_set_config(me, CFG_SETTINGS, cfg)`. If `err` non-NULL: show it on the status/footer line, stay in CONFIG_EDIT. If NULL: `midend_new_game` + reload + PLAYING.
- `` ` `` cancels (no apply).
- Always `free_cfg(cfg)` on leaving the state (apply or cancel). The editor owns a mutable copy of the `sval` strings it grows; free those it allocated.

## Command menu (Tab)

`drawPicker` over entries, each with a hotkey suffix:

| Entry | Suffix | Action |
|---|---|---|
| Size / Type ▸ | | → TYPE_MENU |
| New game | Ctrl+N | `midend_new_game` + reload |
| Restart | Ctrl+R | `midend_restart_game` |
| Solve | (none) | `midend_solve` (menu-only, deliberate: no accidental give-up) |
| Undo | Ctrl+Z | `midend_process_key(UI_UNDO)` |
| Redo | Ctrl+Y | `midend_process_key(UI_REDO)` |
| Pointer | On/Off | toggle `g_ptr_on` |

`;`/`.` scroll, Enter activate, `` ` ``/Tab close. Hotkey hints spell `Ctrl` (not `^`).

## Note-taking (verified, no special handling)

Solo/Keen: Enter (`CURSOR_SELECT`) toggles pencil↔ink; digits commit per mode; Space (`CURSOR_SELECT2`)/`\b` erase. Map: Enter = fill, Space = stipple (colour note). All reachable with the keymap above — no extra keys.

## Discoverability

`Tab` is the universal meta key but invisible until taught. Three affordances:

1. **Chooser footer hint** — picker footer on the game chooser reads `Enter: play · Tab: help`.
2. **`HELP` screen** (chooser `Tab`) — static controls reference: cursor `;./,/`, Enter/Space (select/select2), `[`/`]` clicks, `Ctrl+Z/Y/N/R`, `` ` `` back, and **"Tab: menu (in game)"**. Documents the non-guessable keys (Ctrl-hotkeys, brackets). `` ` `` returns to chooser. (Spec 2 grows chooser `Tab` into Help + Settings.)
3. **Self-extinguishing in-game toast** — on game start, if `g_tab_seen` (RAM bool) is false, draw a small `Tab ▸ menu` toast over the canvas (like the crosshair) for ~2 s, then auto-hide via the `g_last_ms` timer. Pressing `Tab` (which opens COMMAND) sets `g_tab_seen = true`, so the toast never shows again that session. A new player sees it each game only until they discover Tab once; everyone else never sees it. Re-arms per power-on (cross-boot suppression = trivial NVS flag in Spec 2).

## Rendering details

- All picker/menu/editor screens draw to the existing `M5Canvas` then push (reuse `frontend.cpp` plumbing) OR draw directly to `M5.Display` like the current menu. Follow the current menu's direct-`M5.Display` approach for chooser/menus; in-game stays canvas-based. Keep `d_start`'s `clearClipRect` (existing fix).
- Fonts: header/body 8 px (size 1), selected row 12 px (size between 1 and 2 via `setTextSize` + manual, or size 2 scaled). Match the approved mockups.

## Testing

Host-testable (pure midend) — extend `startgame_test`:
1. For each game: flatten presets, assert ≥ 1 leaf.
2. For each game: cycle every preset (`set_params` + `new_game`), assert no crash/hang (under run timeout).
3. For each game: `get_config(CFG_SETTINGS)` → `set_config` round-trip with values unchanged → expect NULL error + `new_game` succeeds; `free_cfg`.
4. Existing 40-game lifecycle + re-entry stress stays.

Device-only (manual): picker render/scroll/wrap, Type menu apply, Custom editor field editing + validation error display, command menu, Ctrl-hotkeys, `[`/`]` clicks, note-taking spot-check (Solo/Map).

## Out of scope → Spec 2

Favorites/star (needs NVS), global device settings (brightness/volume/pointer-default via Tab-in-chooser), per-game remembered settings, NVS persistence module. All reuse `picker.h` + the config editor from this spec.
