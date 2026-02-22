#ifndef GAME_H
#define GAME_H

#define BOARD_ROWS 20
#define BOARD_COLS 10

#define NUM_PIECES 7;

typedef struct {
  unsigned char rows[4];
  int color_id;
} Tetromino;

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

void init_game(GameState *game);
void spawn_piece(GameState *game);
void rotate(GameState *game, int dir);
void move(GameState *game, int dir);
int fall(GameState *game);
void drop(GameState *game);
void store(GameState *game);
void update(GameState *game, double delta_time);

#endif
