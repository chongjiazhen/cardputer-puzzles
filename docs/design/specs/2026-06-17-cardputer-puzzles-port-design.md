# Cardputer Puzzles Port — Design

**Date:** 2026-06-17
**Status:** Approved (brainstorming), pre-implementation
**Target:** M5Stack Cardputer ADV (ESP32-S3 Stamp-S3A, 8MB flash, ~512KB SRAM, **no PSRAM**), 240×135 display, QWERTY keyboard + IMU.

## Goal

Port Simon Tatham's Portable Puzzle Collection to the Cardputer ADV by writing a new
**Cardputer frontend** for the upstream collection. Reuse Tatham's actual C game code
(MIT-licensed) via the puzzles `drawing_api` + game-agnostic midend; do not rewrite games.
Ship as a public GitHub repo others can build and flash.

Success (v1): one `puzzles` firmware containing a curated set of games, selectable from an
on-device menu, playable with the keyboard, buildable + flashable from a clean checkout.

## Non-goals (v1)

- All ~40 puzzles. Curated subset first; expand later.
- Save/resume of in-progress games to SD/NVS. (Possible later; YAGNI now.)
- Editing upstream game logic. Frontend is purely additive.
- Network / WUI delivery. USB-C flash from PlatformIO.

## 1. Approach & repo

- New standalone repo **`cardputer-puzzles`** (e.g. `chongjiazhen/cardputer-puzzles`).
  Kept **private until v1 milestone M1**, then flipped public.
- Upstream `puzzles` vendored as a **git submodule** → `git.tartarus.org/~simon/puzzles.git`.
  Never edited. Not forked (ghewgill mirror is stale/unofficial; submodule tracks Simon's releases).
- All our code additive in `src/puzzles/`. MIT license carried + attribution in README.
- Own `platformio.ini` (board `m5stack-stamps3`, framework arduino, M5Cardputer).
- Own vendored copy of `cardputer_hw` (display/keyboard/IMU/brightness/volume wrapper).
  Accept minor drift vs the private monorepo copy — it is small and stable.

## 2. Architecture

Three layers. Bottom two are upstream-unchanged C.

### a. Games + midend (upstream, unchanged)
Per-game `*.c` implement `const game thegame` (new_game, make_move/interpret_move, redraw,
solve, etc.). `midend.c` drives game state, undo/redo, timer, redraw invalidation, presets.
Compiled as-is.

### b. Frontend glue (`frontend.cpp` / `.h`)
Implements the puzzles `drawing_api` on a single **240×135×16bpp `M5Canvas` backbuffer**
(~64KB SRAM; no PSRAM needed).
- `draw_rect`, `draw_line`, `draw_polygon` (convex fan via fillTriangle), `draw_circle`,
  `draw_text`, `clip`/`unclip` (canvas clip rect), `draw_thick_line`, `draw_update`
  (push canvas → LCD), `start_draw`/`end_draw`.
- **Blitter** (region save/load) reads/writes canvas rectangles — trivial because everything
  renders to the backbuffer. Also gives tear-free output.
- Colours: midend gives RGB floats 0..1 per index → precompute RGB565 once per `new_game`.
- Frontend functions: `fatal`, `frontend_default_colour`, `get_random_seed`
  (esp_random + millis), `activate_timer`/`deactivate_timer` (millis-based).
- Text: map requested pixel-height + FONT_FIXED/VARIABLE → nearest M5GFX font scale.

### c. App shell (`main.cpp`)
M5 init → game-chooser menu → per-game event loop:
`midend_new` → `midend_new_game` → `midend_size(240,135,...)` → loop {
read input → `midend_process_key` → `midend_timer` (millis) → `midend_redraw` }.

### Multi-game in one firmware
Replicate upstream's combined build: compile each selected game `.c` with the macro that
renames `thegame` to a unique per-game symbol; generate a small `gamelist.c`:
`extern const game net, solo, …; const game *const games[] = {…};`.
Menu indexes `games[]`. **Primary plumbing risk — see §7.**

## 3. Curated game list (v1, ~8)

All native keyboard-cursor + small default grid, fit 240×135:

**Net, Mines, Solo (Sudoku), Light Up, Filling, Bridges, Unequal (Futoshiki), Tents.**

Skipped (edge/drag-click-heavy): Loopy, Slant, Untangle, Map — revisit once tilt-pointer proven.

## 4. Input

Both active simultaneously.

### Keyboard → midend cursor
- Arrows → `CURSOR_UP/DOWN/LEFT/RIGHT`
- Enter → `CURSOR_SELECT` (left click), Space → `CURSOR_SELECT2` (right click)
- Digits / letters → passthrough (per-game: fill numbers, etc.)
- `u`/`z` undo, `r` redo, `n` new game, `Esc` back to menu, `?` help overlay

### IMU tilt pointer
- Gravity vector → pointer velocity past a deadzone, clamped to screen bounds.
- Click key → `LEFT_BUTTON` / `RIGHT_BUTTON` at pointer position.
- Cursor sprite drawn **on top** of canvas at push time (not into the canvas, so blitter
  ignores it).
- Toggle key freezes the pointer (anti-drift). Unlocks mouse-only puzzles in later versions.

## 5. Testing

Lightweight spec + pragmatic TDD. Host-runnable unit tests (pattern:
`ebook/rsvp_test.cpp`) for the pure layers:
- key → midend-event mapping
- IMU pointer kinematics (deadzone / clamp / velocity)
- colour float → RGB565
- font-height → M5GFX scale selection

Drawing glue + hardware behaviour: manual on-device. Game logic is upstream, already tested by
Tatham's own suite.

## 6. Milestones & expose plan

- **M1**: firmware builds; menu lists games; **Solo or Net playable** via keyboard on-device.
  → flip repo public (README + attribution + build/flash docs).
- **M2**: all 8 playable; IMU tilt pointer working.
- Post-M2: save/resume, expand game set (incl. mouse-only via pointer), per-game presets UI.

## 7. Risks

1. **Combined-game build macro** — `thegame` rename + generated list under PlatformIO (no CMake).
   *Mitigation: prototype with one game first; get build green before adding the rest.*
2. **Font sizing** on 240×135 — puzzles sizes text to cells; may look cramped.
   *Mitigation: nearest-scale mapping, tune per game.*
3. **Submodule source selection** — point PlatformIO at specific upstream `.c` + supply any
   headers CMake would normally generate (version, config stubs).
   *Mitigation: resolve during M1 on the single-game prototype.*

## Repo layout (target)

```
cardputer-puzzles/
  platformio.ini            # env:puzzles, board m5stack-stamps3
  README.md                 # build/flash, attribution, license
  LICENSE                   # MIT (carry upstream)
  upstream/                 # git submodule -> git.tartarus.org puzzles
  lib/cardputer_hw/         # vendored display/keyboard/IMU wrapper
  src/puzzles/
    main.cpp                # M5 init, menu, event loop, IMU pointer
    frontend.cpp / .h       # drawing_api impl on M5Canvas + frontend glue
    input.cpp / .h          # key -> event (host-testable)
    pointer.cpp / .h        # IMU kinematics (host-testable)
    gamelist.c              # generated: extern const game ...; games[]
    input_test.cpp
    pointer_test.cpp
  tools/
    gen_gamelist.*          # produce gamelist.c + per-game build flags
  docs/design/specs/        # this design + future specs
```
