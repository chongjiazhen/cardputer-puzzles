#pragma once
#include <M5Cardputer.h>
#include "input.h"

namespace puz {

// Translate a single Cardputer ADV keypress character to an InputEvent.
// Returns {Ev::None, 0} for unmapped keys.
//
// ARROW KEYS — CONFIRMED-ON-DEVICE REQUIRED:
//   The ';' '.' ',' '/' cluster is used here per the plan. On the ADV, Fn +
//   these keys may report different codes. The actual keycodes must be
//   verified on hardware; this is the single place to adjust them.
//   If any of these chars are needed as literal puzzle input, remap arrows to
//   whatever Fn-modified code M5Cardputer actually reports for the ADV.
inline InputEvent eventForChar(char c) {
  switch (c) {
    case ';': return {Ev::Up, 0};       // ADV arrow cluster (unconfirmed — verify on device)
    case '.': return {Ev::Down, 0};
    case ',': return {Ev::Left, 0};
    case '/': return {Ev::Right, 0};
    case '\r': case '\n': return {Ev::Select, 0};
    case ' ': return {Ev::Select2, 0};
    case '\b': case 0x7f: return {Ev::Char, '\b'};
    case 'u': return {Ev::Undo, 0};
    case 'r': return {Ev::Redo, 0};
    case 'n': return {Ev::NewGame, 0};
    case 's': return {Ev::Solve, 0};
    case '`': return {Ev::Menu, 0};     // Esc-equivalent -> back to chooser (Task 8)
    case 'k': return {Ev::ClickL, 0};
    case 'l': return {Ev::ClickR, 0};
    case 'p': return {Ev::TogglePointer, 0};
    default:
      if (c >= '0' && c <= '9') return {Ev::Char, c};
      if (c >= 'a' && c <= 'z') return {Ev::Char, c};
      if (c >= 'A' && c <= 'Z') return {Ev::Char, c};
      return {Ev::None, 0};
  }
}

}  // namespace puz
