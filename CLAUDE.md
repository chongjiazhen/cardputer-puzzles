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

## Device gotchas (host tests don't catch these — no stack/flash limits on host)
- Heavy backtracking generators (Solo + others in the re-entry stress set) recurse deep in `midend_new_game`. Arduino's default 8KB loop-task stack → **instant panic on generate despite ~300KB free heap**. Fix: `SET_LOOP_TASK_STACK_SIZE(32*1024)` at file scope in `main.cpp`. Symptom = instant reset, no `fatal()` screen; the *stack* is the wall, not the heap.
- USB-CDC Serial drops across reset/power-off and `pio device monitor` won't reconnect — useless for boot/crash logs. Instrument with **on-screen** diagnostics (`M5.Display`, keypress-gated) instead.
- `getenv_bool` (upstream `misc.c`) tests only the first char against `yYtT` — set env values to `"y"`, never `"1"`.
- Game struct: `->name` is the display name (has spaces: "Light Up"); `->htmlhelp_topic` (3rd field) is a unique lowercase slug ("lightup") — use it for filenames/keys.
- Per-game save → `<htmlhelp_topic>.sav` via a backend picked at boot: **SD card** (`/puzzles/`, pins SCK40/MISO39/MOSI14/CS12) if present, else internal **LittleFS** (`spiffs` partition from `default_8MB.csv`). SD-first is what makes persistence work under **SD-launcher / app-only installs** — they run under the launcher's partition table, which has no LittleFS partition; full-flash installs without a card use LittleFS. Don't flip to LittleFS-first: `begin(true)` could reformat the launcher's own `spiffs` partition. Full-flash merged bin = esptool `merge-bin`, S3 bootloader @ 0x0.
