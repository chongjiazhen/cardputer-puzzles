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

See the **[Controls table in the README](../README.md#controls)** — that is the
single source of truth. In brief: every plain key goes to the puzzle; frontend
commands live on `Ctrl`, `Tab`, and `` ` ``. The `;` `.` `,` `/` cluster moves
the cursor; `Enter`/`Space` select; `Tab` opens the in-game menu (size/type, new,
restart, solve, undo/redo, pointer). All mappings are defined in
`src/puzzles/keymap.h` — the single place to remap keys.

### IMU tilt pointer

Press `Ctrl+P` to toggle the gyroscope/accelerometer tilt pointer. When active,
tilting the device moves a crosshair cursor on screen; press `Enter` or `Space`
to click (left / right) at the crosshair. Useful for games that take direct
coordinate input (e.g. Mines, Light Up).

---

## Included games

40 of Tatham's puzzles ship in v1.0 — browse them in the on-device picker, or see
the full list in the [README](../README.md#included-games-40).

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
