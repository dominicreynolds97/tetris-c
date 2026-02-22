#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include "src/game.h"
#include "src/input.h"
#include "src/renderer.h"

int is_cell_filled(GameState *game, int row, int col) {
  return game->board[row][col] != 0;
}

int main(void) {
  GameState game;

  setup_renderer();
  init_game(&game);
  srand(time(NULL));

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
