# cardputer-puzzles

Simon Tatham's Portable Puzzle Collection ported to the **M5Stack Cardputer ADV** (ESP32-S3 Stamp-S3A).

Upstream project: https://www.chiark.greenend.org.uk/~sgtatham/puzzles/

## Included games (39)

Simon Tatham's collection, alphabetical:

Black Box · Bridges · Cube · Dominosa · Fifteen · Filling · Flip · Flood ·
Guess · Inertia · Keen · Light Up · Loopy · Magnets · Map · Mines · Mosaic ·
Net · Netslide · Palisade · Pattern · Pearl · Pegs · Range · Rectangles ·
Same Game · Signpost · Singles · Sixteen · Slant · Solo · Tents · Towers ·
Train Tracks · Twiddle · Undead · Unequal · Unruly · Untangle

> **Galaxies** is temporarily excluded: its generator exhausts the S3's SRAM
> even at small sizes. It returns once the render canvas drops to an 8bpp
> palette (frees ~32 KB). Any game that does run out of memory shows an error
> and reboots to the menu rather than hanging.

## Hardware

- M5Stack Cardputer ADV (ESP32-S3 Stamp-S3A, 8 MB flash, **no PSRAM**)
- 240x135 display
- Primary / tested target: Cardputer ADV

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
| `Tab` | **Menu** (in game: size/type, new, restart, solve, undo/redo, pointer) · **Help** (on the game list) |
| `p` | Toggle the IMU tilt pointer (then Enter/Space click at the crosshair) |
| `` ` `` | Back (to the game list / out of a menu) |

The game list and menus are a scrolling picker: `;`/`.` move, `Enter` selects,
`` ` `` backs out. Press `Tab` in a game for the menu; `Tab` on the list for help.

> The `;` `.` `,` `/` cursor cluster is confirmed on the ADV. `src/puzzles/keymap.h`
> is the single place to remap keys.

## Attribution

Game logic and midend © Simon Tatham and contributors, vendored **unmodified** as a git submodule (`upstream/`) pinned at commit `7ad37c64`. Only the Cardputer frontend (`src/puzzles/`, `lib/`) is original to this project.

See [LICENSE](LICENSE) for the full MIT licence text.
