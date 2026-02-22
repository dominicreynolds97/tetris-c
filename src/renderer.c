#include "SDL_render.h"
#include "SDL_surface.h"
#include "SDL_video.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include "renderer.h"
#include "input.h"

typedef struct { int r, g, b; } Color;

Color COLORS[] = {
  {0,   0,   0  }, // 0 - empty
  {255, 0,   0  }, // 1 - S
  {0,   255, 0  }, // 2 - Z
  {255, 255, 0  }, // 3 - L
  {0,   0,   255}, // 4 - J
  {128, 0,   255}, // 5 - T
  {0,   255, 255}, // 6 - I
  {255, 255, 255}, // 7 - O
};


SDL_Window *window;
SDL_Renderer *renderer;
TTF_Font *font;

void setup_renderer(void) {
  SDL_Init(SDL_INIT_VIDEO);
  TTF_Init();
  window = SDL_CreateWindow("Tetris", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  font = TTF_OpenFont(FONT_LOCATION, FONT_SIZE);
}

void cleanup_renderer(void) {
  TTF_CloseFont(font);
  TTF_Quit();
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

void draw_border(int x, int y, int w, int h) {
  SDL_Rect rect = { x, y, w, h };
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderDrawRect(renderer, &rect);
}

void render_cell_at(int x, int y, int size, int color_id) {
  SDL_Rect rect = { x, y, size, size };
  Color color = COLORS[color_id];
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
  SDL_RenderFillRect(renderer, &rect);

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 50);
  SDL_RenderDrawRect(renderer, &rect);
}

void render_text(const char *text, int x, int y) {
  SDL_Color white = {255, 255, 255, 255};
  SDL_Surface *surface = TTF_RenderText_Solid(font, text, white);
  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

  SDL_Rect dst = { x, y, surface->w, surface->h };
  SDL_RenderCopy(renderer, texture, NULL, &dst);

  SDL_FreeSurface(surface);
  SDL_DestroyTexture(texture);
}

void render_cell(int row, int col, int cell_value) {
  render_cell_at(
      MARGIN + col * CELL_SIZE,
      MARGIN + row * CELL_SIZE,
      CELL_SIZE,
      cell_value
  );
}

void render_stored(GameState *game) {
  draw_border(PANEL_X, MARGIN, 4 * PANEL_CELL_SIZE + 2, 4 * PANEL_CELL_SIZE + 2);

  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 4; c++) {

      int color_id = game->has_stored_piece
        ? game->stored_peice.cells[r][c]
        : 0;

      render_cell_at(
          PANEL_X + 1 + c * PANEL_CELL_SIZE,
          MARGIN + 1 + r * PANEL_CELL_SIZE,
          PANEL_CELL_SIZE,
          color_id
      );
    }
  }
}

void clear_frame(void) {
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);
}

void render_ui(GameState *game) {
  char buf[64];

  snprintf(buf, sizeof(buf), "Score: %d", game->score);
  render_text(buf, PANEL_X, MARGIN + 4 * PANEL_CELL_SIZE + 30);

  snprintf(buf, sizeof(buf), "Level: %d", game->level);
  render_text(buf, PANEL_X, MARGIN + 4 * PANEL_CELL_SIZE + 50);

  snprintf(buf, sizeof(buf), "Lines: %d", game->total_clears);
  render_text(buf, PANEL_X, MARGIN + 4 * PANEL_CELL_SIZE + 70);
}

void render(GameState *game) {
  clear_frame();

  draw_border(MARGIN - 1, MARGIN - 1, BOARD_COLS * CELL_SIZE + 2, BOARD_ROWS * CELL_SIZE + 2);

  for (int r = 0; r < BOARD_ROWS; r++) {
    for (int c = 0; c < BOARD_COLS; c++) {
      int pr = r - game->piece_row;
      int pc = c - game->piece_col;

      int is_piece_cell = pr >= 0 && pr < 4 &&
                          pc >= 0 && pc < 4 &&
                          game->active_piece.cells[pr][pc] != 0;

      int color_id;
      if (is_piece_cell) {
        color_id = game->active_piece.cells[pr][pc];
      } else {
        color_id = game->board[r][c];
      }
      render_cell(r, c, color_id);
    }
  }

  render_stored(game);

  render_ui(game);

  SDL_RenderPresent(renderer);
}

void render_game_over(GameState *game) {
  clear_frame();

  draw_border(MARGIN, MARGIN, SCREEN_WIDTH - MARGIN - MARGIN, SCREEN_HEIGHT - MARGIN - MARGIN);
  render_text("GAME OVER", SCREEN_WIDTH / 2 - 80, SCREEN_HEIGHT / 2 - 80);

  char buf[64];

  snprintf(buf, sizeof(buf), "Score: %d", game->score);
  render_text(buf, SCREEN_WIDTH / 2 - 60, SCREEN_HEIGHT / 2 - 30);

  snprintf(buf, sizeof(buf), "Level: %d", game->level);
  render_text(buf, SCREEN_WIDTH / 2 - 60, SCREEN_HEIGHT / 2);

  snprintf(buf, sizeof(buf), "Lines: %d", game->total_clears);
  render_text(buf, SCREEN_WIDTH / 2 - 60, SCREEN_HEIGHT / 2 + 30);

  render_text("Press R to restart or Q to quit", SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 + 70);

  SDL_RenderPresent(renderer);

  handle_game_over_input(game);
}
