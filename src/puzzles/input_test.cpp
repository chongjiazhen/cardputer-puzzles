// input_test.cpp — g++ -std=c++17 input_test.cpp -o input_test && ./input_test
#include "input.h"
#include <cassert>
#include <cstdio>
using puz::Ev; using puz::InputEvent; using puz::midendButton;

// midend constants mirrored for the host test.
// Values verified against upstream/puzzles.h enum (lines 32-63):
//   LEFT_BUTTON=0x0200 counting up → CURSOR_UP=0x0209, CURSOR_DOWN=0x020a,
//   CURSOR_LEFT=0x020b, CURSOR_RIGHT=0x020c, CURSOR_SELECT=0x020d,
//   CURSOR_SELECT2=0x020e, UI_LOWER_BOUND=0x020f, UI_QUIT=0x0210,
//   UI_NEWGAME=0x0211, UI_SOLVE=0x0212, UI_UNDO=0x0213, UI_REDO=0x0214
enum {
  CURSOR_UP     = 0x0209,
  CURSOR_DOWN   = 0x020a,
  CURSOR_LEFT   = 0x020b,
  CURSOR_RIGHT  = 0x020c,
  CURSOR_SELECT = 0x020d,
  CURSOR_SELECT2= 0x020e,
  UI_NEWGAME    = 0x0211,
  UI_SOLVE      = 0x0212,
  UI_UNDO       = 0x0213,
  UI_REDO       = 0x0214
};

int main() {
  assert(midendButton({Ev::Up,      0})   == CURSOR_UP);
  assert(midendButton({Ev::Down,    0})   == CURSOR_DOWN);
  assert(midendButton({Ev::Left,    0})   == CURSOR_LEFT);
  assert(midendButton({Ev::Right,   0})   == CURSOR_RIGHT);
  assert(midendButton({Ev::Select,  0})   == CURSOR_SELECT);
  assert(midendButton({Ev::Select2, 0})   == CURSOR_SELECT2);
  assert(midendButton({Ev::Undo,    0})   == UI_UNDO);
  assert(midendButton({Ev::Redo,    0})   == UI_REDO);
  assert(midendButton({Ev::NewGame, 0})   == UI_NEWGAME);
  assert(midendButton({Ev::Solve,   0})   == UI_SOLVE);
  assert(midendButton({Ev::Char,   '5'})  == '5');
  assert(midendButton({Ev::Char,   'a'})  == 'a');
  assert(midendButton({Ev::Menu,          0}) == -1);
  assert(midendButton({Ev::None,          0}) == -1);
  assert(midendButton({Ev::ClickL,        0}) == -1);
  assert(midendButton({Ev::ClickR,        0}) == -1);
  assert(midendButton({Ev::TogglePointer, 0}) == -1);
  printf("ok\n");
  return 0;
}
