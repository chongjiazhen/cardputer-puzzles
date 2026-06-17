# cardputer-puzzles

Simon Tatham's Portable Puzzle Collection ported to the **M5Stack Cardputer ADV** (ESP32-S3 Stamp-S3A).

Upstream project: https://www.chiark.greenend.org.uk/~sgtatham/puzzles/

## Included games

| Game | Notes |
|---|---|
| **Net** | Rotate tiles to connect all nodes |
| **Mines** | Minesweeper |
| **Solo** | Sudoku |
| **Light Up** | Place lightbulbs to illuminate every cell |
| **Filling** | Fill regions with digits matching region size |
| **Bridges** | Connect islands with the right number of bridges |
| **Unequal** | Futoshiki — place digits respecting inequality signs |
| **Tents** | Place tents next to trees satisfying row/column counts |

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

Requires [PlatformIO](https://platformio.org/) and the ESP32 toolchain (installed automatically by PlatformIO on first build).

Copy `secrets.h.example` to `src/puzzles/secrets.h` and fill in Wi-Fi credentials if Wi-Fi features are used; otherwise leave blank.

### Host tests (Linux / CI)

Unit tests for pure logic (key mapping, pointer kinematics, colour conversion) can be run on the host without hardware:

```sh
g++ -std=c++17 src/puzzles/<name>_test.cpp -o /tmp/test && /tmp/test
```

Note: the primary development box uses MSVC; the public CI target is g++.

## Controls

| Key | Action |
|---|---|
| `;` | Cursor up |
| `.` | Cursor down |
| `,` | Cursor left |
| `/` | Cursor right |
| `Enter` | Left-click / select |
| `Space` | Right-click / select2 |
| digits / letters | Per-game entry |
| `u` | Undo |
| `r` | Redo |
| `n` | New game |
| `s` | Solve |
| `k` | Pointer left-click |
| `l` | Pointer right-click |
| `p` | Toggle IMU tilt pointer |
| `` ` `` | Back to game menu |

> **Note:** The `;` `.` `,` `/` arrow-key mapping is provisional. On the ADV the Fn-modified key codes must be confirmed on hardware. `src/puzzles/keymap.h` is the single place to adjust them.

## Attribution

Game logic and midend © Simon Tatham and contributors, vendored **unmodified** as a git submodule (`upstream/`) pinned at commit `7ad37c64`. Only the Cardputer frontend (`src/puzzles/`, `lib/`) is original to this project.

See [LICENSE](LICENSE) for the full MIT licence text.
