#include "input.h"
#include "player.h"

#include "vendor/imgui/imgui.h"
#include <SDL3/SDL.h>

#define DEADZONE 0.10
#define MAX_GAMEPADS 8

bool scancodesDown[SDL_SCANCODE_COUNT];
SDL_Gamepad *gamepads[8];
int numOpenedGamepads = 0;


bool Input::IsScanDown(SDL_Scancode scancode)
{
    return scancodesDown[scancode];
}


void Input::HandleEvent(SDL_Event *event)
{
    if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP) {
        scancodesDown[event->key.scancode] = event->key.down;
    }
    else if (event->type == SDL_EVENT_GAMEPAD_ADDED) {
        SDL_JoystickID joyId = event->gdevice.which;
        gamepads[numOpenedGamepads] = SDL_OpenGamepad(joyId);
        if (gamepads[numOpenedGamepads] == NULL) {
            SDL_Log("Could not open gamepad with Joystick id %d: %s", 
                    joyId, SDL_GetError());
        }
        else {
            numOpenedGamepads++;
        }
    }
    else if (event->type == SDL_EVENT_GAMEPAD_REMOVED) {
        SDL_JoystickID joyId = event->gdevice.which;
        for (int i = 0; i < MAX_GAMEPADS; i++) {
            if (SDL_GetGamepadID(gamepads[i]) == joyId) {
                // Move last gamepad to location of deleted gamepad to keep them
                // all at the start of the array
                SDL_CloseGamepad(gamepads[i]);
                gamepads[i] = gamepads[numOpenedGamepads];
                gamepads[numOpenedGamepads] = NULL; // Not necessary
                numOpenedGamepads--;
                break;
            }
        }
    }
}


float Input::GetScanAxis(SDL_Scancode negScan, SDL_Scancode posScan)
{
    bool isDownNeg = Input::IsScanDown(negScan);
    bool isDownPos = Input::IsScanDown(posScan);
    return (float)isDownPos - (float)isDownNeg;
}


float Input::GetGamepadAxis(SDL_Gamepad *gamepad, SDL_GamepadAxis axis)
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
    return gamepads[0];
}


bool Input::GetGamepadButton(SDL_Gamepad *gamepad, SDL_GamepadButton button)
{
    return SDL_GetGamepadButton(gamepad, button);
}


void Input::DebugGUI()
{
    ImGui::Begin("Input");
    for (int i = 0; i < numOpenedGamepads; i++) {
        ImGui::Text("%s %d, player idx %d", SDL_GetGamepadName(gamepads[i]),
                    i, SDL_GetGamepadPlayerIndex(gamepads[i]));
    }

    char comboTitle[32];
    for (int i = 0; i < NUM_PLAYERS; i++) {
        SDL_snprintf(comboTitle, 32, "Player %i Input Device", i);

        char comboValueString[32];
        if (gPlayers[i].gamepad != nullptr) {
            const char *gamepadName = SDL_GetGamepadName(gPlayers[i].gamepad);
            //SDL_JoystickID gamepadID = SDL_GetGamepadID(gPlayers[i].gamepad);
            SDL_snprintf(comboValueString, 32, "%s %d",
                         gamepadName, SDL_GetGamepadID(gPlayers[i].gamepad));
        }
        else {
            SDL_snprintf(comboValueString, 32, "Use keyboard");
        }
        
        if (!ImGui::BeginCombo(comboTitle, comboValueString)) {
            continue;
        }
        // Option for keyboard (by settings gamepad to nullptr)
        const bool isKeyboardSelected = gPlayers[i].gamepad == nullptr;
        if (ImGui::Selectable("Use keyboard", isKeyboardSelected)) {
            gPlayers[i].gamepad = nullptr;
        }
        // Options for gamepads
        for (int j = 0; j < numOpenedGamepads; j++) {
            SDL_snprintf(comboValueString, 32, "%s %d",
                         SDL_GetGamepadName(gamepads[j]),
                         SDL_GetGamepadID(gamepads[j]));
            const bool isSelected = gamepads[j] == gPlayers[i].gamepad;
            if (ImGui::Selectable(comboValueString, isSelected)) {
                gPlayers[i].gamepad = gamepads[j];
            }
        }
        ImGui::EndCombo();
    }
    ImGui::End();
}

