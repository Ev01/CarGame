#pragma once

#include <SDL3/SDL.h>


namespace MainGame {
    SDL_AppResult Init();
    SDL_AppResult HandleEvent(SDL_Event *event);
    SDL_AppResult Update();
    void CleanUp();
};
