#include "input.h"
#include "SDL_events.h"
#include "game.h"

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

void handle_game_over_event(GameState *game) {
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
