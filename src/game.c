#include "game.h"
#include <stdlib.h>

Tetromino pieces[] = {
  {{ 0b0000, 0b0110, 0b0011, 0b0000 }, 1}, // S
  {{ 0b0000, 0b0011, 0b0110, 0b0000 }, 2}, // Z
  {{ 0b0000, 0b0100, 0b0111, 0b0000 }, 3}, // L
  {{ 0b0000, 0b0001, 0b0111, 0b0000 }, 4}, // J
  {{ 0b0000, 0b0010, 0b0111, 0b0000 }, 5}, // T
  {{ 0b0000, 0b0000, 0b1111, 0b0000 }, 6}, // I
  {{ 0b0000, 0b0110, 0b0110, 0b0000 }, 7}  // O
};

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



void rotate(GameState *game, int dir) {
  TetrominoState rotated = {0};
  int nudge = 0;
  int can_rotate = 1;

  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 4; c++) {
      if (game->active_piece.cells[r][c]) {
        int new_c = (dir == 1) ? (3 - r) : r;
        if (game->piece_col + new_c < 0 &&
            nudge < -(game->piece_col + new_c)) {
          nudge = -(game->piece_col + new_c);
        }
        if (game->piece_col + new_c >= BOARD_COLS &&
            nudge > BOARD_COLS - 1 - (game->piece_col + new_c)) {
          nudge = BOARD_COLS - 1 - (game->piece_col + new_c);
        }
      }
    }
  }

  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 4; c++) {
      if (game->active_piece.cells[r][c]) {
        int new_r = (dir == 1) ? c : (3 - c);
        int new_c = (dir == 1) ? (3 - r) : r;

        if (game->piece_row + new_r >= 0 &&
             game->board[game->piece_row + new_r][game->piece_col + new_c + nudge] != 0
        ) {
          can_rotate = 0;
          break;
        }
        rotated.cells[new_r][new_c] = game->active_piece.cells[r][c];
      }
    }
    if (can_rotate == 0) break;
  }
  if (can_rotate == 1) {
    game->piece_col = nudge;
    game->active_piece = rotated;
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

