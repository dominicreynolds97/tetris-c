#ifndef INPUT_H
#define INPUT_H

#include "game.h"

extern volatile int running;

void handle_input(GameState *game);
void handle_game_over_input(GameState *game);

#endif
