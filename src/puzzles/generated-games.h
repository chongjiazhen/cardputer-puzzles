/* generated-games.h — hand-crafted for cardputer-puzzles; lists all curated games compiled in.
 * Expand GAME(x) macros for each puzzle compiled in.
 * In a full CMake build this would be auto-generated from the puzzle list.
 */
GAME(net)
GAME(mines)
GAME(solo)
GAME(lightup)
GAME(filling)
GAME(bridges)
GAME(unequal)
GAME(tents)
GAME(blackbox)
GAME(cube)
GAME(dominosa)
GAME(fifteen)
GAME(flip)
GAME(flood)
GAME(galaxies)
GAME(guess)
/* hat excluded: hat.c is tiling library code, not a puzzle game */
GAME(inertia)
GAME(keen)
GAME(loopy)
GAME(magnets)
/* map excluded from GAME() macro — Arduino declares long map(...) globally,
 * conflicting with "extern const game map" in C++ translation units.
 * gamelist.c declares it directly (plain C, no Arduino header in scope). */
GAME(mosaic)
GAME(netslide)
GAME(palisade)
GAME(pattern)
GAME(pearl)
GAME(pegs)
GAME(range)
GAME(rect)
GAME(samegame)
GAME(signpost)
GAME(singles)
GAME(sixteen)
GAME(slant)
/* spectre excluded: spectre.c is tiling library code, not a puzzle game */
GAME(towers)
GAME(tracks)
GAME(twiddle)
GAME(undead)
GAME(unruly)
GAME(untangle)
