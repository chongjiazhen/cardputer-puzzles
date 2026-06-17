#include "puzzles.h"

extern const game net, mines, solo, lightup, filling, bridges, unequal, tents;

const game *gamelist[] = {
  &net, &mines, &solo, &lightup, &filling, &bridges, &unequal, &tents
};
const int gamecount = sizeof(gamelist) / sizeof(gamelist[0]);
