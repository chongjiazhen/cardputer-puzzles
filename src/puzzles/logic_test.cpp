// logic_test.cpp — host tests for pure frontend logic (no midend, no Arduino).
// Build: g++ -std=c++17 -I./src/puzzles src/puzzles/logic_test.cpp -o logic_test && ./logic_test
#include <cassert>
#include <cstdio>
#include <vector>
#include "input.h"
#include "keymap.h"
#include "picker.h"

static int fails = 0;
#define CHECK(c) do { if(!(c)){ printf("FAIL %s:%d  %s\n",__FILE__,__LINE__,#c); fails++; } } while(0)

static void test_buildKeyPresses() {
    using namespace puz;
    auto a = buildKeyPresses({'z'}, true, false, false, false);
    CHECK(a.size()==1 && a[0].ch=='z' && a[0].ctrl==true);

    auto b = buildKeyPresses({'a','b'}, false, false, false, false);
    CHECK(b.size()==2 && b[0].ch=='a' && !b[0].ctrl && b[1].ch=='b');

    auto e = buildKeyPresses({}, false, true, false, false);
    CHECK(e.size()==1 && e[0].ch=='\r' && !e[0].ctrl);

    auto d = buildKeyPresses({}, false, false, true, false);
    CHECK(d.size()==1 && d[0].ch=='\b');

    auto t = buildKeyPresses({}, false, false, false, true);
    CHECK(t.size()==1 && t[0].ch=='\t');
}

static void test_eventForKey() {
    using namespace puz;
    CHECK(eventForKey({';',false}).kind == Ev::Up);
    CHECK(eventForKey({'/',false}).kind == Ev::Right);
    CHECK(eventForKey({'\r',false}).kind == Ev::Select);
    CHECK(eventForKey({' ',false}).kind == Ev::Select2);
    CHECK(eventForKey({'[',false}).kind == Ev::Char);   // brackets now fall through to the game
    CHECK(eventForKey({']',false}).kind == Ev::Char);
    CHECK(eventForKey({'\t',false}).kind == Ev::CommandMenu);
    CHECK(eventForKey({'`',false}).kind == Ev::BackToChooser);
    CHECK(eventForKey({'z',true}).kind == Ev::Undo);
    CHECK(eventForKey({'y',true}).kind == Ev::Redo);
    CHECK(eventForKey({'n',true}).kind == Ev::NewGame);
    CHECK(eventForKey({'r',true}).kind == Ev::Restart);
    CHECK(eventForKey({'p',true}).kind == Ev::TogglePointer);   // Ctrl+P toggles the tilt pointer
    CHECK(eventForKey({'p',false}).kind == Ev::Char);           // plain 'p' still belongs to the game
    // plain letters/digits go to the game as Char (nothing stolen)
    InputEvent g = eventForKey({'l',false});
    CHECK(g.kind == Ev::Char && g.ch == 'l');
    CHECK(eventForKey({'5',false}).kind == Ev::Char);
    CHECK(eventForKey({'a',false}).kind == Ev::Char);
}

static void test_pickerWindow() {
    using namespace puz;
    PickerWindow a = pickerWindow(40, 0, 7);   // wrap at top
    CHECK(a.count==7 && a.selSlot==3 && a.idx[0]==37 && a.idx[3]==0 && a.idx[6]==3);

    PickerWindow b = pickerWindow(40, 39, 7);  // wrap at bottom
    CHECK(b.count==7 && b.selSlot==3 && b.idx[3]==39 && b.idx[4]==0);

    PickerWindow c = pickerWindow(3, 1, 7);    // fewer than rows: show all, no wrap
    CHECK(c.count==3 && c.selSlot==1 && c.idx[0]==0 && c.idx[2]==2);

    PickerWindow d = pickerWindow(40, 20, 7);  // middle
    CHECK(d.count==7 && d.idx[0]==17 && d.idx[3]==20 && d.idx[6]==23);
}

int main() {
    test_buildKeyPresses();
    test_eventForKey();
    test_pickerWindow();
    printf(fails ? "\n%d FAIL\n" : "\nlogic_test ok\n", fails);
    return fails ? 1 : 0;
}
