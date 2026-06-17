#include "puzzles.h"

extern const game net;

const game *gamelist[] = { &net };
const int gamecount = sizeof(gamelist) / sizeof(gamelist[0]);
