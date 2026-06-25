#include "puzzles.h"

extern const game net, mines, solo, lightup, filling, bridges, unequal, tents;
extern const game blackbox, cube, dominosa, fifteen, flip, flood, galaxies, guess;
extern const game inertia, keen, loopy, magnets, mosaic, netslide;
extern const game map;  /* declared separately: GAME(map) omitted from generated-games.h (Arduino name conflict) */
extern const game palisade, pattern, pearl, pegs, range, rect, samegame, signpost;
extern const game singles, sixteen, slant, towers, tracks, twiddle;
extern const game undead, unruly, untangle;

/* hat.c and spectre.c are tiling library code, not puzzle games — no const game hat/spectre */

/* Menu order: alphabetical by display name, matching the Android port and the
 * upstream website. Display names differ from symbol names: rect="Rectangles",
 * samegame="Same Game", tracks="Train Tracks", lightup="Light Up". */
const game *gamelist[] = {
  /* galaxies is back: it was never a real OOM — its generator makes a zero-size
   * allocation, and newlib malloc(0) returns NULL (glibc doesn't), which Tatham's
   * smalloc misreads as out-of-memory. Fixed by the malloc(0)->1 shim
   * (src/puzzles/malloc0_shim.c + -Wl,--wrap=malloc). */
  &blackbox, &bridges, &cube, &dominosa, &fifteen, &filling, &flip, &flood,
  &galaxies, &guess, &inertia, &keen, &lightup, &loopy, &magnets, &map,
  &mines, &mosaic, &net, &netslide, &palisade, &pattern, &pearl, &pegs,
  &range, &rect, &samegame, &signpost, &singles, &sixteen, &slant, &solo,
  &tents, &towers, &tracks, &twiddle, &undead, &unequal, &unruly, &untangle,
};
const int gamecount = sizeof(gamelist) / sizeof(gamelist[0]);
