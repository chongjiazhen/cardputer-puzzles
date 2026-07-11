# cardputer-puzzles

Simon Tatham's Portable Puzzle Collection ported to the **M5Stack Cardputer ADV** (ESP32-S3 Stamp-S3A).

Upstream project: https://www.chiark.greenend.org.uk/~sgtatham/puzzles/

## Included games (40)

Simon Tatham's collection, alphabetical:

Black Box · Bridges · Cube · Dominosa · Fifteen · Filling · Flip · Flood ·
Galaxies · Guess · Inertia · Keen · Light Up · Loopy · Magnets · Map · Mines · Mosaic ·
Net · Netslide · Palisade · Pattern · Pearl · Pegs · Range · Rectangles ·
Same Game · Signpost · Singles · Sixteen · Slant · Solo · Tents · Towers ·
Train Tracks · Twiddle · Undead · Unequal · Unruly · Untangle

> Any game that runs out of memory shows an error and reboots to the menu
> rather than hanging.

## Hardware

- M5Stack Cardputer ADV (ESP32-S3 Stamp-S3A, 8 MB flash, **no PSRAM**)
- 240x135 display
- Primary / tested target: Cardputer ADV

## Install (prebuilt)

Grab a `.bin` from the [latest release](../../releases/latest) — no toolchain needed.
Both assets carry the version in the name (`…-vX.Y-…`); a flashed device also shows it
in the `Tab`→Help screen.

- **Full flash (recommended)** — `cardputer-puzzles-vX.Y-merged.bin`, written at offset `0x0`.
  Self-contained (bootloader + partition table + app) and the only path that guarantees
  **save-state persistence**, since it installs the LittleFS data partition this firmware
  expects. Flash via [M5Burner](https://docs.m5stack.com/en/download) ("Custom firmware"),
  a web flasher, or esptool:
  ```sh
  esptool --chip esp32s3 write-flash 0x0 cardputer-puzzles-vX.Y-merged.bin
  ```
- **App-only** — `cardputer-puzzles-vX.Y-app.bin`, for OTA / SD-card launchers such as the
  [M5Stack Launcher](https://bmorcelli.github.io/Launcher/). Runs fine, but it keeps the
  device's existing partition table — resume/persistence works **only** if that table has a
  `spiffs` data partition. For guaranteed persistence, use the full-flash bin above.

## Building and flashing

```sh
# Clone with submodule (game logic lives in upstream/)
git clone --recurse-submodules <repo-url>
cd cardputer-puzzles

# Or, after a plain clone:
git submodule update --init

# Build and flash
pio run -e puzzles -t upload
```

Requires [PlatformIO](https://platformio.org/) and the ESP32 toolchain (installed automatically by PlatformIO on first build). No secrets or network config — the puzzles run fully offline.

### Host tests

`startgame_test` exercises the full midend lifecycle (new game → size → redraw → free) for all 40 games on a desktop. Verified 40/40 pass before every flash.

Build (two-step: C files via gcc, C++ via g++):

```sh
# 1. Compile all C sources
gcc -std=c11 -DCOMBINED -DNO_TGMATH_H -I./upstream -I./src/puzzles -c \
    upstream/combi.c upstream/divvy.c upstream/draw-poly.c \
    upstream/drawing.c upstream/dsf.c upstream/findloop.c \
    upstream/grid.c upstream/hat.c upstream/latin.c upstream/laydomino.c \
    upstream/loopgen.c upstream/malloc.c upstream/matching.c \
    upstream/midend.c upstream/misc.c upstream/penrose.c \
    upstream/penrose-legacy.c upstream/printing.c upstream/ps.c \
    upstream/random.c upstream/sort.c upstream/spectre.c \
    upstream/tdq.c upstream/tree234.c upstream/version.c \
    upstream/net.c upstream/mines.c upstream/solo.c upstream/lightup.c \
    upstream/filling.c upstream/bridges.c upstream/unequal.c upstream/tents.c \
    upstream/blackbox.c upstream/cube.c upstream/dominosa.c \
    upstream/fifteen.c upstream/flip.c upstream/flood.c \
    upstream/galaxies.c upstream/guess.c upstream/inertia.c upstream/keen.c \
    upstream/loopy.c upstream/magnets.c upstream/map.c upstream/mosaic.c \
    upstream/netslide.c upstream/palisade.c upstream/pattern.c \
    upstream/pearl.c upstream/pegs.c upstream/range.c \
    upstream/rect.c upstream/samegame.c upstream/signpost.c \
    upstream/singles.c upstream/sixteen.c upstream/slant.c \
    upstream/towers.c upstream/tracks.c upstream/twiddle.c \
    upstream/undead.c upstream/unruly.c upstream/untangle.c \
    src/puzzles/gamelist.c

# 2. Compile and link C++ test
g++ -std=c++17 -DCOMBINED -DNO_TGMATH_H -I./upstream -I./src/puzzles \
    -c src/puzzles/startgame_test.cpp -o startgame_test.o
g++ -std=c++17 startgame_test.o *.o -o startgame_test -lm && ./startgame_test
```

Note: `hat.c` and `spectre.c` are tiling libraries (not games) — compiled in for `grid.c` but not listed in `gamelist[]`.

## Controls

Every plain key goes to the puzzle (so games that use letters — Map's `l`,
Solo's hex digits, etc. — work). Frontend commands live on Ctrl, Tab, and `` ` ``.

| Key | Action |
|---|---|
| `;` `.` `,` `/` | Move cursor (up / down / left / right) |
| `Enter` | Select (cursor cell, or click at the crosshair in pointer mode) |
| `Space` | Select2 / mark (flag, pencil, …) |
| digits / letters / `⌫` | Per-game input |
| `Ctrl`+`Z` / `Ctrl`+`Y` | Undo / Redo |
| `Ctrl`+`N` / `Ctrl`+`R` | New game / Restart |
| `Tab` | **Menu** (in game: size/type, new, restart, solve, undo/redo, pointer, recenter, zoom, pan, rules) · **Help** (on the game list) |
| `Ctrl`+`P` | Toggle the IMU tilt pointer (then Enter/Space act at the crosshair) |
| `Ctrl`+`Space` | Recenter the tilt pointer — set the current hold as neutral (works at any posture) |
| `Ctrl`+`L` | Toggle 2× zoom — play continues magnified; `Ctrl`+`;` `.` `,` `/` pans the view |
| `` ` `` | Back (to the game list / out of a menu) |

**Map** colours by drag: `Enter` picks up the colour under the cursor/crosshair,
move, `Enter` drops it (`Space` = pencil). In pointer mode the pickup is held
across two presses (source → destination).

The game list and menus are a scrolling picker: `;`/`.` move, `Enter` selects,
`` ` `` backs out, and a scrollbar tracks your position. Press `Tab` in a game
for the menu; `Tab` on the list for help.

> The `;` `.` `,` `/` cursor cluster is confirmed on the ADV. `src/puzzles/keymap.h`
> is the single place to remap keys.

## Roadmap

v1.1.1 ships the 40 games above with an on-device picker, a `Tab` command menu
with an in-menu rules viewer, preset/custom sizing, an 8bpp palette render
canvas, a battery indicator, idle backlight dimming that deepens into
light-sleep, **per-game resume that survives power-off** (saved to the SD card,
or internal flash on full-flash installs — so it persists under SD launchers
too), tilt-pointer recenter, and a non-bricking error path. Planned next:

- **Favorites / star** a game, pinned to the top of the list.
- **Settings** — brightness, volume, default pointer (via `Tab` on the game list).
- **Remember per-game size** — persist the chosen preset/custom dimensions too.

## Known limitations

A few games have rendering rough edges, tracked as issues:

- **Concave shapes render wrong** — Signpost / Net arrows ([#1](../../issues/1)).
- **Drag leaves smear trails** — Untangle / Sixteen / Twiddle / Netslide ([#2](../../issues/2)).
- **Small default-preset digits** — Mines / Filling / Pattern / Flood ([#3](../../issues/3)).

## Attribution

Game logic and midend © Simon Tatham and contributors, vendored **unmodified** as a git submodule (`upstream/`) pinned at commit `7ad37c64`. Only the Cardputer frontend (`src/puzzles/`, `lib/`) is original to this project.

The frontend is MIT — see [LICENSE](LICENSE). The vendored puzzles keep their own MIT licence and copyright in [upstream/LICENCE](upstream/LICENCE).
