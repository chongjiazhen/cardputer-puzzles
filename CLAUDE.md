# cardputer-puzzles — Claude notes

Custom C++ frontend for Simon Tatham's Portable Puzzle Collection on the M5Stack Cardputer ADV (PlatformIO/ESP32-S3).

## Scope
- `upstream/` — Simon Tatham's collection (git submodule): all game logic + the midend, vendored **unmodified**. Don't edit it; keep the submodule clean.
- A bug in a puzzle's *rules* belongs to upstream, not this repo — and the collection is decades-stable, so such bugs are rare. Don't patch game logic locally; if a rule fix is ever truly unavoidable, carry it as an explicit, documented patch, never a silent submodule edit.
- Frontend (this repo's code) = `src/puzzles/` + `lib/cardputer_hw/`. `gamelist.c` / `generated-games.h` are generated.

## Per-game behavior
- Each game's behavior is governed by flags in its `upstream/<game>.c` game struct (e.g. `wants_statusbar`).
- For any per-game UI work, grep the flag across **all** games first and key off it — don't fix game-by-game.

## Build / test
- Build: `pio run -e puzzles` · Flash: `pio run -e puzzles -t upload` (single env `puzzles`).
- Host tests cover pure logic/lifecycle only; they do **not** compile `frontend.cpp`/`main.cpp` (M5GFX, device-only). Render/input changes need on-hardware verification.
- Draw primitives translate puzzle coords by `fe->offX/offY` (board centering); recompute via `sizeAndCenter()` after any `midend_size` change.
