#include "SDL_events.h"
#include "SDL_render.h"
#include "SDL_surface.h"
#include "SDL_video.h"
#include <_stdio.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <stdlib.h>
#include <SDL.h>
#include <SDL_ttf.h>

#define BOARD_ROWS 20
#define BOARD_COLS 10

#define SCREEN_WIDTH 600
#define SCREEN_HEIGHT 700
#define MARGIN 20
#define PANEL_X (MARGIN + BOARD_COLS * CELL_SIZE + MARGIN)
#define PANEL_CELL_SIZE 25
#define CELL_SIZE 30

#define FONT_LOCATION "/System/Library/Fonts/Helvetica.ttc"
#define FONT_SIZE 20


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

static volatile int running = 1;

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

typedef struct {
  unsigned char rows[4];
  int color_id;
} Tetromino;

Tetromino pieces[] = {
  {{ 0b0000, 0b0110, 0b0011, 0b0000 }, 1}, // S
  {{ 0b0000, 0b0011, 0b0110, 0b0000 }, 2}, // Z
  {{ 0b0000, 0b0100, 0b0111, 0b0000 }, 3}, // L
  {{ 0b0000, 0b0001, 0b0111, 0b0000 }, 4}, // J
  {{ 0b0000, 0b0010, 0b0111, 0b0000 }, 5}, // T
  {{ 0b0000, 0b0000, 0b1111, 0b0000 }, 6}, // I
  {{ 0b0000, 0b0110, 0b0110, 0b0000 }, 7}  // O
};

#define NUM_PIECES 7;

typedef struct {
  int cells[4][4];
} TetrominoState;

typedef struct {
  char board[BOARD_ROWS][BOARD_COLS];
  int score;
  int level;
  int total_clears;
  TetrominoState active_piece;
  TetrominoState stored_peice;
  int has_stored_piece;
  char piece_row;
  char piece_col;
  float speed;
  int game_over;
} GameState;

TetrominoState make_state(Tetromino *piece) {
  TetrominoState state = {0};
  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 4; c++) {
      if ((piece->rows[r] >> (3 - c)) & 1) {
        state.cells[r][c] = piece->color_id;
      }
    }
  }
  return state;
}

void rotate(GameState *game, int dir) {
  TetrominoState rotated = {0};
  int can_rotate = 1;
  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 4; c++) {
      if (game->active_piece.cells[r][c]) {
        int new_r = (dir == 1) ? c : (3 - c);
        int new_c = (dir == 1) ? (3 - r) : r;
        if (
            game->piece_col + new_c < 0 ||
            game->piece_col + new_c >= BOARD_COLS || (
             game->piece_row + new_r >= 0 &&
             game->board[game->piece_row + new_r][game->piece_col + new_c] != 0
        )) {
          can_rotate = 0;
          break;
        }
        rotated.cells[new_r][new_c] = game->active_piece.cells[r][c];
      }
    }
    if (can_rotate == 0) break;
  }
  if (can_rotate == 1) {
    game->active_piece = rotated;
  }
}

void spawn_piece(GameState *game) {
  int idx = rand() % NUM_PIECES;
  game->active_piece = make_state(&pieces[idx]);
  game->piece_row = -3;
  game->piece_col = BOARD_COLS / 2 - 2;
}

void init_game(GameState *game) {
  for (int r = 0; r < BOARD_ROWS; r++) {
    for (int c = 0; c < BOARD_COLS; c++) {
      game->board[r][c] = 0;
    }
  }
  game->score = 0;
  game->level = 1;
  game->total_clears = 0;
  game->has_stored_piece = 0;
  game->game_over = 0;
  game->speed = 1.0;
}

int is_cell_filled(GameState *game, int row, int col) {
  return game->board[row][col] != 0;
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

int detect_collision(GameState *game) {
  for (int pr = 0; pr < 4; pr++) {
    for (int pc = 0; pc < 4; pc++) {
      if (game->active_piece.cells[pr][pc] != 0) {
        int next_row = game->piece_row + pr + 1;
        if (next_row < 0) continue;
        if (next_row >= BOARD_ROWS) return 1;
        if (game->board[next_row][pc + game->piece_col] != 0) return 1;
      }
    }
  }
  return 0;
}

void trigger_game_over(GameState *game) {
  game->game_over = 1;
}

int handle_collision(GameState *game) {
  for (int pr = 0; pr < 4; pr++) {
    for (int pc = 0; pc < 4; pc++) {
      if (game->active_piece.cells[pr][pc] != 0) {
        if (game->piece_row + pr <= 0) {
          trigger_game_over(game);
          return 0;
        }
        game->board[pr + game->piece_row][pc + game->piece_col] = game->active_piece.cells[pr][pc];
      }
    }
  }
  return 1;
}

void handle_sigint(int sig) {
  (void)sig;
  running = 0;
}

int can_move(GameState *game, int dir) {
  for (int pr = 0; pr < 4; pr++) {
    for (int pc = 0; pc < 4; pc++) {
      if (game->active_piece.cells[pr][pc] != 0) {
        int row = pr + game->piece_row;
        // detect bounds
        if (game->piece_col + pc + dir < 0 || game->piece_col + pc + dir >= BOARD_COLS) return 0;

        if (row < 0) continue;

        // detect collision
        if (game->board[pr + game->piece_row][pc + game->piece_col + dir] != 0) return 0;
      }
    }
  }

  return 1;
}

void move(GameState *game, int dir) {
  if (can_move(game, dir) != 0) {
    game->piece_col = game->piece_col + dir;
  }
}

int fall(GameState *game) {
  int has_collided = detect_collision(game);
  if (has_collided != 0) {
    if (handle_collision(game)) {
      spawn_piece(game);
    }
  } else {
    game->piece_row += 1;
  }
  return has_collided;
}

void drop(GameState *game) {
  int has_collided = 0;
  while (has_collided == 0) {
    has_collided = fall(game);
  }
}

void store(GameState *game) {
  if (game->has_stored_piece) {
    TetrominoState temp = game->stored_peice;
    game->stored_peice = game->active_piece;
    game->active_piece = temp;
    game->piece_row = -3;
    game->piece_col = BOARD_COLS / 2 - 2;
  } else {
    game->stored_peice = game->active_piece;
    game->has_stored_piece = 1;
    spawn_piece(game);
  }
}

void delete_row(GameState *game, int row) {
  for (int c = 0; c < BOARD_COLS; c++) {
    game->board[row][c] = 0;
  }
}

void move_row_down(GameState *game, int row, int offset) {
  if (offset == 0) return;

  for (int c = 0; c < BOARD_COLS; c++) {
    int old = game->board[row][c];
    game->board[row + offset][c] = old;
    game->board[row][c] = 0;
  }
}

void detect_and_handle_full_rows(GameState *game) {
  int offset = 0;
  // go from bottom to top
  for (int r = BOARD_ROWS - 1; r >=0; r--) {
    int filled = 1;
    for (int c = 0; c < BOARD_COLS; c++) {
      if (game->board[r][c] == 0) {
        filled = 0;
        break;
      }
    }

    if (filled == 1) {
      offset += 1;
      delete_row(game, r);
    } else {
      move_row_down(game, r, offset);
    }
  }

  game->total_clears += offset;

  game->score += offset * offset * game->level;

  if (game->total_clears - (game->level * 10) >= 0) {
    game->level += 1;
    game->speed = game->speed * 0.9;
  }
}

void update(GameState *game, double delta_time) {
  (void)delta_time;
  fall(game);
  detect_and_handle_full_rows(game);
}

void handle_input(GameState *game) {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) running = 0;
    if (event.type == SDL_KEYDOWN) {
      switch (event.key.keysym.sym) {
        case SDLK_a: move(game, -1); break;
        case SDLK_d: move(game, 1); break;
        case SDLK_s: fall(game); break;
        case SDLK_e: rotate(game, 1); break;
        case SDLK_q: rotate(game, -1); break;
        case SDLK_w: store(game); break;
        case SDLK_SPACE: drop(game); break;
      }
    }
  }
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

  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) running = 0;
    if (event.type == SDL_KEYDOWN) {
      switch (event.key.keysym.sym) {
        case SDLK_r: init_game(game); break;
        case SDLK_q: running = 0; break;

      }
    }
  }
}

int main(void) {
  GameState game;

  setup_renderer();
  init_game(&game);
  srand(time(NULL));
  spawn_piece(&game);

  struct timespec previous, current;
  clock_gettime(CLOCK_MONOTONIC, &current);

  double accumulator = 0.0;

  signal(SIGINT, handle_sigint);

  while (running) {
    clock_gettime(CLOCK_MONOTONIC, &current);

    double delta_time = (current.tv_sec - previous.tv_sec) + (current.tv_nsec - previous.tv_nsec) / 1e9;

    previous = current;
    accumulator += delta_time;

    if (game.game_over == 0) {
      handle_input(&game);
      render(&game);

      if (accumulator >= game.speed) {
        update(&game, accumulator);
        accumulator = 0.0;
      }
    } else {
      render_game_over(&game);
    }
  }

  cleanup_renderer();

  return 0;
}
