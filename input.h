#pragma once

#include <SDL3/SDL.h>

namespace Input
{
    bool IsScanDown(SDL_Scancode scancode);
    // Return 1.0f if posScan is pressed, -1.0f if negScan is pressed, and 0.0f
    // if neither or both keys are pressed. Can be used for left/right or
    // up/down keys.
    float GetScanAxis(SDL_Scancode negScan, SDL_Scancode posScan);
    float GetGamepadAxis(SDL_GamepadAxis axis);
    bool GetGamepadButton(SDL_GamepadButton button);
    void HandleEvent(SDL_Event *event);
    SDL_Gamepad* GetGamepad();
}
