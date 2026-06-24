#pragma once
#include "input.h"   // pure — no Arduino/M5 deps, so this header is host-testable

namespace puz {

// Modifier-aware mapping (collision-free). Plain digits/letters/'\b' fall
// through to the game as Char; frontend commands live only on Ctrl+key,
// Tab, and '`'.
//
// ARROW KEYS — CONFIRMED-ON-DEVICE REQUIRED:
//   The ';' '.' ',' '/' cluster is used here per the plan. On the ADV, Fn +
//   these keys may report different codes. The actual keycodes must be
//   verified on hardware; this is the single place to adjust them.
//   If any of these chars are needed as literal puzzle input, remap arrows to
//   whatever Fn-modified code M5Cardputer actually reports for the ADV.
inline InputEvent eventForKey(KeyPress k) {
  if (k.ctrl) {
    switch (k.ch) {
      case 'z': case 'Z': return {Ev::Undo, 0};
      case 'y': case 'Y': return {Ev::Redo, 0};
      case 'n': case 'N': return {Ev::NewGame, 0};
      case 'r': case 'R': return {Ev::Restart, 0};
      case 'p': case 'P': return {Ev::TogglePointer, 0};  // plain 'p' belongs to the game
      default:            return {Ev::None, 0};   // unknown Ctrl combo: ignore
    }
  }
  switch (k.ch) {
    case ';': return {Ev::Up, 0};
    case '.': return {Ev::Down, 0};
    case ',': return {Ev::Left, 0};
    case '/': return {Ev::Right, 0};
    case '\r': case '\n': return {Ev::Select, 0};
    case ' ':  return {Ev::Select2, 0};
    case '\t': return {Ev::CommandMenu, 0};
    case '`':  return {Ev::BackToChooser, 0};
    default:   return {Ev::Char, k.ch};   // digits, letters, '\b' → game
  }
}

}  // namespace puz
