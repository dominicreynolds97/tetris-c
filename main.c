#include <_stdio.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

// CONSTANTS
#define BOARD_ROWS 20
#define BOARD_COLS 10
#define HIDE_CURSOR "\e[?25l"
#define SHOW_CURSOR "\e[?25h"
#define CLEAR_CONSOLE "\033[2J"
#define RESET_CURSOR_POS "\033[H"
#define COLOR_RESET "\33[0m"
#define WHITE "\033[37m"

struct termios original_termios;

void enable_raw_mode(void) {
  tcgetattr(STDIN_FILENO, &original_termios);
  struct termios raw = original_termios;
  raw.c_lflag &= ~(ICANON | ECHO);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCIFLUSH, &raw);
}

void disable_raw_mode(void) {
  tcsetattr(STDIN_FILENO, TCIFLUSH, &original_termios);
}

const char *COLORS[] = {
  COLOR_RESET, // 0 - empty, reset
  "\033[31m", // 1 - S, red
  "\033[32m", // 2 - Z, green
  "\033[33m", // 3 - L, yellow
  "\033[34m", // 4 - J, blue
  "\033[35m", // 5 - T, magenta
  "\033[36m", // 6 - I, cyan
  WHITE, // 7 - O, white
};

static volatile int running = 1;

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
        if (game->piece_row + new_r >= 0 && game->board[game->piece_row + new_r][game->piece_col + new_c] != 0) {
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
  game->speed = 1.0;
}

int is_cell_filled(GameState *game, int row, int col) {
  return game->board[row][col] != 0;
}

void render_cell(int cell_value) {
  if (cell_value == 0) {
    printf("  ");
  } else {
    printf("%s██", COLORS[cell_value]);
  }
}

void print_h_border() {
  for (int i = 0; i <= BOARD_COLS; i++) {
    printf("%s--", WHITE);
  }
  printf("\n");
}

void print_stored(GameState *game) {
  printf("%s\n----------\n", WHITE);
  for (int r = 0; r < 4; r++) {
    printf("%s|", WHITE);
    for (int c = 0; c < 4; c++) {
      int cell_value = game->has_stored_piece ? game->stored_peice.cells[r][c] : 0;
      render_cell(cell_value);
    }
    printf("%s|\n", WHITE);
  }
  printf("%s----------\n", WHITE);
}

void render(GameState *game) {
  printf(RESET_CURSOR_POS);

  print_h_border();

  for (int r = 0; r < BOARD_ROWS; r++) {
    printf("%s|", WHITE);
    for (int c = 0; c < BOARD_COLS; c++) {
      int pr = r - game->piece_row;
      int pc = c - game->piece_col;

      int is_piece_cell = pr >= 0 && pr < 4 &&
                          pc >= 0 && pc < 4 &&
                          game->active_piece.cells[pr][pc] != 0;

      if (is_piece_cell) {
        int id = game->active_piece.cells[pr][pc];
        render_cell(id);
      } else {
        int id = game->board[r][c];
        render_cell(id);
      }
    }
    printf("%s|\n", WHITE);
  }
  print_h_border();
  printf("%sScore: %d Level: %d\n", WHITE, game->score, game->level);

  print_stored(game);
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

void handle_collision(GameState *game) {
  for (int pr = 0; pr < 4; pr++) {
    for (int pc = 0; pc < 4; pc++) {
      if (game->active_piece.cells[pr][pc] != 0) {
        game->board[pr + game->piece_row][pc + game->piece_col] = game->active_piece.cells[pr][pc];
      }
    }
  }
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
        if (row < 0) continue;

        // detect bounds
        if (game->piece_col + pc + dir < 0 || game->piece_col + pc + dir >= BOARD_COLS) return 0;

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
    handle_collision(game);
    spawn_piece(game);
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
  char c;
  if (read(STDIN_FILENO, &c, 1) != 1) return;

  switch (c) {
    case 'a':
      // move left
      move(game, -1);
      break;
    case 'd':
      // move right
      move(game, 1);
      break;
    case 's':
      fall(game);
      break;
    case 'e':
      rotate(game, 1);
      break;
    case 'q':
      rotate(game, -1);
      break;
    case 'w':
      store(game);
      break;
    case ' ':
      drop(game);
      break;
  }
}

int main(void) {
  GameState game;

  enable_raw_mode();
  init_game(&game);
  srand(time(NULL));
  spawn_piece(&game);

  printf(CLEAR_CONSOLE);
  printf(HIDE_CURSOR);

  struct timespec previous, current;
  clock_gettime(CLOCK_MONOTONIC, &current);

  double accumulator = 0.0;

  signal(SIGINT, handle_sigint);

  while (running) {
    clock_gettime(CLOCK_MONOTONIC, &current);

    double delta_time = (current.tv_sec - previous.tv_sec) + (current.tv_nsec - previous.tv_nsec) / 1e9;

    previous = current;
    accumulator += delta_time;

    handle_input(&game);
    render(&game);

    if (accumulator >= game.speed) {
      update(&game, accumulator);
      accumulator = 0.0;
    }
  }

  printf(CLEAR_CONSOLE);
  printf(RESET_CURSOR_POS);
  printf(SHOW_CURSOR);
  disable_raw_mode();

  return 0;
}
