# Cardputer Puzzles — Controls Reference and Build Guide

Simon Tatham's Portable Puzzle Collection on the M5Stack Cardputer ADV.

Upstream: https://www.chiark.greenend.org.uk/~sgtatham/puzzles/

---

## Hardware constraints

- **Device:** M5Stack Cardputer ADV (ESP32-S3 Stamp-S3A)
- **Flash:** 8 MB (no PSRAM)
- **Display:** 240×135 pixels
- Game state and rendering run entirely from internal SRAM; no PSRAM dependency.

---

## Controls

All key mappings are defined in `src/puzzles/keymap.h` — that is the single file to edit if keycodes need adjustment.

| Key | Action |
|---|---|
| `;` | Cursor up |
| `.` | Cursor down |
| `,` | Cursor left |
| `/` | Cursor right |
| `Enter` (`\r` / `\n`) | Left-click / select |
| `Space` | Right-click / select2 |
| Digits `0`–`9` | Per-game digit entry |
| Letters `a`–`z`, `A`–`Z` | Per-game letter entry |
| `u` | Undo |
| `r` | Redo |
| `n` | New game |
| `s` | Solve (where the game supports it) |
| `k` | Pointer left-click (at current pointer position) |
| `l` | Pointer right-click (at current pointer position) |
| `p` | Toggle IMU tilt pointer on/off |
| `` ` `` | Back to game chooser menu |

### Arrow key note (ADV provisional mapping)

The `;` `.` `,` `/` cluster is used for cursor up/down/left/right because those are the characters the Cardputer ADV keyboard reports for those physical keys in the current firmware. **This mapping must be confirmed on hardware.** If the ADV reports different codes (e.g. Fn-modified sequences), update `keymap.h` — it is the single place to change.

### IMU tilt pointer

Press `p` to toggle the gyroscope/accelerometer tilt pointer. When active, tilting the device moves a crosshair cursor on screen. Press `k` or `l` to click at the pointer position. This is useful for games that take direct coordinate input (e.g. Mines, Light Up).

---

## Included games

| Game | Description |
|---|---|
| **Net** | Rotate tiles to connect all nodes to the central hub |
| **Mines** | Minesweeper — flag or uncover cells by mine count clues |
| **Solo** | Sudoku — fill grids so each digit appears once per row, column, and block |
| **Light Up** | Place lightbulbs to illuminate every white cell without bulbs seeing each other |
| **Filling** | Fill each region with a digit equal to the region's area |
| **Bridges** | Connect islands with bridges so each island has the right count |
| **Unequal** | Futoshiki — place digits respecting the inequality signs between cells |
| **Tents** | Place tents adjacent to trees, satisfying row and column tent counts |

---

## Build and flash

### Prerequisites

- [PlatformIO](https://platformio.org/) (CLI or VS Code extension)
- USB-C cable to Cardputer ADV
- ESP32 Arduino toolchain (PlatformIO installs this automatically)

### Clone

```sh
git clone --recurse-submodules <repo-url>
cd cardputer-puzzles
```

If you already cloned without `--recurse-submodules`:

```sh
git submodule update --init
```

The `upstream/` submodule is pinned at commit `7ad37c64af3bc891372ec16db1531cee599b6e3a` from `https://git.tartarus.org/simon/puzzles.git`. Do not modify files under `upstream/`.

### Build and upload

```sh
pio run -e puzzles -t upload
```

PlatformIO will compile the puzzles app and flash it to the connected Cardputer ADV over USB.

### Host tests

Pure-logic unit tests (key mapping, pointer kinematics, colour conversion) can be run on the host without hardware:

```sh
# Linux / CI (g++)
g++ -std=c++17 src/puzzles/<name>_test.cpp -o /tmp/test && /tmp/test
```

These tests cover the frontend glue only; the upstream puzzle logic is not separately tested here.

---

## Attribution

Game logic and midend code © Simon Tatham and contributors — vendored unmodified under `upstream/` at the submodule pin above. See [upstream/LICENCE](../upstream/LICENCE) for the full author list and MIT licence text.

Cardputer ADV frontend (`src/puzzles/`, `lib/`) is original to this project and is also MIT-licensed. See [LICENSE](../LICENSE).
