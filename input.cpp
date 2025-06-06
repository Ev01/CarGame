#include "input.h"

#include <SDL3/SDL.h>

#define DEADZONE 0.10

bool scancodesDown[SDL_SCANCODE_COUNT];
SDL_Gamepad *gamepad;


bool Input::IsScanDown(SDL_Scancode scancode)
{
    return scancodesDown[scancode];
}


void Input::HandleEvent(SDL_Event *event)
{
    if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP) {
        scancodesDown[event->key.scancode] = event->key.down;
    }
    else if (event->type == SDL_EVENT_GAMEPAD_ADDED && gamepad == NULL) {
        SDL_JoystickID joyId = event->gdevice.which;
        gamepad = SDL_OpenGamepad(joyId);
        if (gamepad == NULL) {
            SDL_Log("Could not open gamepad with Joystick id %d: %s", 
                    joyId, SDL_GetError());
        }
    }
    else if (event->type == SDL_EVENT_GAMEPAD_REMOVED) {
        SDL_JoystickID joyId = event->gdevice.which;
        if (SDL_GetGamepadID(gamepad) == joyId) {
            gamepad = NULL;
        }
    }
}


float Input::GetScanAxis(SDL_Scancode negScan, SDL_Scancode posScan)
{
    bool isDownNeg = Input::IsScanDown(negScan);
    bool isDownPos = Input::IsScanDown(posScan);
    return (float)isDownPos - (float)isDownNeg;
}


float Input::GetGamepadAxis(SDL_GamepadAxis axis)
{
    Sint16 amount = SDL_GetGamepadAxis(gamepad, axis);
    // Normalise the result to between -1.0f and 1.0f. Note that
    // SDL_JOYSTICK_AXIS_MAX is closer to 0 than SDL_JOYSTICK_AXIS_MIN,
    // so doing it this way may result in normAmount being able to go 
    // slightly below -1.0f.
    float normAmount = (float) amount / SDL_JOYSTICK_AXIS_MAX;
    normAmount = normAmount < -1.0f ? -1.0f : normAmount;
    if (SDL_fabsf(normAmount) < DEADZONE) {
        normAmount = 0.0;
    }
    return normAmount;
}


SDL_Gamepad* Input::GetGamepad()
{
    return gamepad;
}


bool Input::GetGamepadButton(SDL_GamepadButton button)
{
    return SDL_GetGamepadButton(gamepad, button);
}

