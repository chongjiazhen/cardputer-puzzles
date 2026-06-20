#include "puzzles.h"

extern const game net, mines, solo, lightup, filling, bridges, unequal, tents;
extern const game blackbox, cube, dominosa, fifteen, flip, flood, galaxies, guess;
extern const game inertia, keen, loopy, magnets, mosaic, netslide;
extern const game map;  /* declared separately: GAME(map) omitted from generated-games.h (Arduino name conflict) */
extern const game palisade, pattern, pearl, pegs, range, rect, samegame, signpost;
extern const game singles, sixteen, slant, towers, tracks, twiddle;
extern const game undead, unruly, untangle;

/* hat.c and spectre.c are tiling library code, not puzzle games — no const game hat/spectre */

const game *gamelist[] = {
  &net, &mines, &solo, &lightup, &filling, &bridges, &unequal, &tents,
  &blackbox, &cube, &dominosa, &fifteen, &flip, &flood, &galaxies, &guess,
  &inertia, &keen, &loopy, &magnets, &map, &mosaic, &netslide,
  &palisade, &pattern, &pearl, &pegs, &range, &rect, &samegame, &signpost,
  &singles, &sixteen, &slant, &towers, &tracks, &twiddle,
  &undead, &unruly, &untangle,
};
const int gamecount = sizeof(gamelist) / sizeof(gamelist[0]);
