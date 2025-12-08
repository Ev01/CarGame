#pragma once

#include <SDL3/SDL.h>

enum GameState {
    GAME_PRESS_START_SCREEN,
    GAME_IN_WORLD,
    GAME_IN_WORLD_PAUSED
};

namespace MainGame {
    SDL_AppResult Init();
    SDL_AppResult HandleEvent(SDL_Event *event);
    SDL_AppResult Update();
    void StartWorld();
    void Quit();
    void CleanUp();

    extern GameState gGameState;
};
