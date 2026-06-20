// logic_test.cpp — host tests for pure frontend logic (no midend, no Arduino).
// Build: g++ -std=c++17 -I./src/puzzles src/puzzles/logic_test.cpp -o logic_test && ./logic_test
#include <cassert>
#include <cstdio>
#include <vector>
#include "input.h"

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

int main() {
    test_buildKeyPresses();
    printf(fails ? "\n%d FAIL\n" : "\nlogic_test ok\n", fails);
    return fails ? 1 : 0;
}
