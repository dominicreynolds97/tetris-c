#ifndef RENDERER_H
#define RENDERER_H

#include "game.h"
#include "input.h"

#define SCREEN_WIDTH 600
#define SCREEN_HEIGHT 700
#define MARGIN 20
#define PANEL_X (MARGIN + BOARD_COLS * CELL_SIZE + MARGIN)
#define PANEL_CELL_SIZE 25
#define CELL_SIZE 30

#define FONT_LOCATION "/System/Library/Fonts/Helvetica.ttc"
#define FONT_SIZE 20

void setup_renderer(void);
void cleanup_renderer(void);
void render(GameState *game);
void render_game_over(GameState *game);

#endif
