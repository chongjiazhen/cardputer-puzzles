# Contributing

Thanks for helping. The most valuable contribution to this project is **playing
the games on a real Cardputer and reporting what you find** — see why below.

## Why playtesting matters most

The logic layer is covered by host tests (`logic_test.cpp` for keymap/picker,
`startgame_test.cpp` for the 39 games' lifecycle + presets + config). Those run
on a desktop and prove the code *generates and doesn't crash*.

They **cannot** see the 240×135 screen or the keyboard. Every rendering, input,
and feel issue in this project was found by playing it on hardware — stale
frames, cramped fonts, a key that didn't reach a game, a flickering overlay.
So: flash it, play each game properly (not just "does it load"), and report.

## How to report — three buckets

When you file an issue, say which kind it is:

1. **Objective bug** — a crash, wrong behaviour, or something that clearly
   doesn't work. Include: game, what you did (key by key if input-related),
   what happened, what you expected. Repro steps beat descriptions.
2. **Feature** — something missing you want. One feature per issue.
3. **Subjective / UX** — feel and legibility: "Keen's digits are too small at
   the default size", "the pointer fights this game", "this menu is confusing".
   These are real and wanted — say which game and why.

Legibility is often tunable without code: per-game starting sizes live in
`src/puzzles/presets.h` (a name → params-string table). If a game is too dense
to read, a smaller preset there usually fixes it — that's a great first PR.

## Dev basics

Build/flash and run the host tests per the [README](README.md). Before a PR:

- **Keep the host tests green** (`logic_test` + `startgame_test`).
- **Flash and playtest** the games your change touches — note in the PR what you
  verified on-device, since CI can't.
- Conventional commit messages (`feat:`, `fix:`, `docs:`…).
- Keep the keymap rule intact: **plain keys belong to the game**; frontend
  commands live on `Ctrl`, `Tab`, and `` ` `` (`src/puzzles/keymap.h`).

## Scope

Game logic + the midend are upstream (Simon Tatham's collection, vendored
unmodified). Bugs in a *puzzle's rules* go upstream; this repo owns the
Cardputer frontend (`src/puzzles/`, `lib/`) — rendering, input, menus, sizing.
