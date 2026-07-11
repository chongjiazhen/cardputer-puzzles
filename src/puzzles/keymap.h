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
      case ' ':           return {Ev::RecenterPointer, 0}; // zero the tilt neutral to the current hold
      case 'l': case 'L': return {Ev::ZoomPeek, 0};       // toggle 2x magnifier (plain 'l' belongs to the game)
      // Ctrl+arrow-cluster pans the zoom window. With ctrl held the M5Cardputer
      // keyboard lib reports value_second (the SHIFTED char) in `word` -- see
      // Keyboard.cpp updateKeysState: ctrl||shift||caps all select value_second.
      // So Ctrl+';' arrives as ':' etc. Match both, same reason the letter
      // hotkeys above match upper AND lower case.
      case ';': case ':': return {Ev::PanUp, 0};
      case '.': case '>': return {Ev::PanDown, 0};
      case ',': case '<': return {Ev::PanLeft, 0};
      case '/': case '?': return {Ev::PanRight, 0};
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
