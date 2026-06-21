# New Games Design — cardputer-puzzles

**Date:** 2026-06-18  
**Scope:** Fix startGame crash; add all 26 remaining upstream puzzle games.

---

## Phase 1 — Crash Fix

### Context

Commit `652a652` added `Serial.printf` traces to `startGame` to diagnose a crash.  
Traces pinpoint the failing call; output collected via serial monitor at 115200.

### Known side-effect to fix first

`setup()` has `while (!Serial) delay(100)` — hangs device when no USB monitor connected. Remove before any other testing.

### Diagnosis table

| Last trace seen | Suspect |
|---|---|
| `SG(n): START` | crash before `midend_free` — bad `g_me` ptr |
| `SG(n): free done` | `midend_new` crashes — allocator failure or bad `gamelist[idx]` |
| `SG(n): new done` | `midend_new_game` — OOM or stack overflow during puzzle gen |
| `SG(n): colours done` | `frontend_load_colours` — bad colour count |
| `SG(n): size done` | `midend_size` — assertion or divide-by-zero |
| `SG(n): redraw done` | `midend_force_redraw` — canvas alloc failure / null drawing ptr |

If OOM suspected (trace stops at `new done`): add `ESP.getFreeHeap()` log after `midend_new` to confirm.

### Fix

Target root cause. Common patterns on ESP32-S3:
- **Heap fragmentation** — `midend_free` + `midend_new` in tight loop fragments 320 KB SRAM. Consider preallocating canvas once.
- **Stack overflow** — puzzle generators recurse; default Arduino task stack is 8 KB. Increase via `CONFIG_ARDUINO_LOOP_STACK_SIZE` if needed.
- **Null return** — `midend_new` can return null on alloc failure; check before calling `midend_new_game`.

### Cleanup (after fix confirmed)

Remove from `main.cpp`:
- All `Serial.printf("SG(...)` traces in `startGame`
- `while (!Serial) delay(100)` in `setup()`
- `Serial.printf("SELECT idx=...")` in `loop()`
- Removed inline comments (already done in `652a652` diff)

---

## Phase 2 — Add 26 Games

### Mechanics

Three files, one commit:

1. **`platformio.ini`** — append `+<../upstream/game.c>` to `build_src_filter`
2. **`src/puzzles/generated-games.h`** — append `GAME(game)`
3. **`src/puzzles/gamelist.c`** — add `extern const game game;` and `&game` to array

### Game list

**Standard (self-contained .c, no extra deps):**

blackbox, cube, dominosa, fifteen, flip, flood, galaxies, guess, inertia, keen,
loopy, magnets, map, mosaic, netslide, palisade, pattern, pearl, pegs, range,
rect, samegame, signpost, singles, sixteen, slant, towers, tracks, twiddle,
undead, unruly, untangle

**Special:**

| Game | Extra headers (already in `upstream/`) | Notes |
|---|---|---|
| `hat` | `hat.h`, `hat-internal.h`, `hat-tables.h` | Self-contained; no auxiliary/ needed |
| `spectre` | `spectre-internal.h`, `spectre-tables-auto.h`, `spectre-tables-manual.h` | Self-contained; no auxiliary/ needed |

Both compile same as standard games. If either exceeds flash/RAM budget, exclude and note in `generated-games.h` comment.

### Flash budget

8 MB flash. Current 8 games ~1–2 MB. 26 additional puzzle `.c` files well within budget.

### Success criteria

- `pio run` completes without error
- All 34 games appear in the menu on device
- No regression in existing 8 games

---

## Out of scope

- Tuning per-game keyboard mappings (existing `keymap.h` handles all games via cursor + select + char passthrough)
- UI changes (menu scrolls already; no change needed)
- `unfinished/` games (group, numgame, path, separate, slide, sokoban) — excluded
