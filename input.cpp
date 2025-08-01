#include "input.h"
#include "player.h"

#include "vendor/imgui/imgui.h"
#include <SDL3/SDL.h>

#define DEADZONE 0.10
#define MAX_GAMEPADS 8

bool scancodesDown[SDL_SCANCODE_COUNT];
SDL_Gamepad *gamepads[8];
int numOpenedGamepads = 0;
//InputMappingKeyboard gDefaultKeyboardMapping;


ControlScheme gDefaultControlScheme;


void InputMapping::AssignMapping(SDL_Scancode aScancode)
{
    mappingType = MAPPING_TYPE_SCANCODE;
    scancode = aScancode;
}
void InputMapping::AssignMapping(SDL_GamepadButton aButton)
{
    mappingType = MAPPING_TYPE_GAMEPAD_BUTTON;
    button = aButton;
}
void InputMapping::AssignMapping(SDL_GamepadAxis aAxis, bool isAxisPos)
{
    if (isAxisPos) {
        mappingType = MAPPING_TYPE_GAMEPAD_AXIS_POS;
    }
    else {
        mappingType = MAPPING_TYPE_GAMEPAD_AXIS_NEG;
    }
    axis = aAxis;
}
float InputMapping::GetValue(SDL_Gamepad *gamepad)
{
    if (gamepad == nullptr && mappingType != MAPPING_TYPE_SCANCODE) {
        return 0.0;
    }

    switch (mappingType) {
        case MAPPING_TYPE_SCANCODE:
            // Only enable keyboard when not given controller to use
            if (gamepad == nullptr) {
                return (float) Input::IsScanDown(scancode);
            }
            break;
        case MAPPING_TYPE_GAMEPAD_AXIS_POS:
            return Input::GetGamepadAxisPos(gamepad, axis);
        case MAPPING_TYPE_GAMEPAD_AXIS_NEG:
            return Input::GetGamepadAxisNeg(gamepad, axis);
        case MAPPING_TYPE_GAMEPAD_BUTTON:
            return Input::GetGamepadButton(gamepad, button);
        case MAPPING_TYPE_NONE:
            return 0.0;
    }

    return 0.0;
}

void InputAction::AddMapping(SDL_Scancode scancode)
{
    numMappingsSet = SDL_min(numMappingsSet, 2);
    mappings[numMappingsSet++].AssignMapping(scancode);
}
void InputAction::AddMapping(SDL_GamepadButton button)
{
    numMappingsSet = SDL_min(numMappingsSet, 2);
    mappings[numMappingsSet++].AssignMapping(button);
}
void InputAction::AddMapping(SDL_GamepadAxis axis, bool isPosAxis)
{
    numMappingsSet = SDL_min(numMappingsSet, 2);
    mappings[numMappingsSet++].AssignMapping(axis, isPosAxis);
}

float InputAction::GetValue(SDL_Gamepad *gamepad) {
    float toReturn = 0.0;
    for (int i = 0; i < numMappingsSet; i++) {
        // Choose the mapping with the greatest input value. This should
        // work fine in most occasions
        toReturn = SDL_max(toReturn, mappings[i].GetValue(gamepad));
    }
    return toReturn;
}


void InputAction::HandleEvent(SDL_Event *event)
{
    /*
    for (int i = 0; i < numMappingsSet; i++) {
        switch (mappings[i].mappingType) {
            case MAPPING_TYPE_SCANCODE:
                if (event->type == SDL_EVENT_KEY_DOWN 
                        || event->type == SDL_EVENT_KEY_UP) {
                    lastPressed = i;
                    return;
                }
                break;

            case MAPPING_TYPE_GAMEPAD_BUTTON:
                if (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN
                        || event->type == SDL_EVENT_GAMEPAD_BUTTON_UP) {
                    lastPressed = i;
                    return;
                }
                break;
            
            case MAPPING_TYPE_GAMEPAD_AXIS_NEG:
            case MAPPING_TYPE_GAMEPAD_AXIS_POS:
                if (event->type == SDL_EVENT_GAMEPAD_AXIS_MOTION) {
                    lastPressed = i;
                    return;
                }
                break;
            case MAPPING_TYPE_NONE:
        }
    }
    */
}


float ControlScheme::GetInputForAction(GameAction action, SDL_Gamepad *gamepad)
{
    return actions[action].GetValue(gamepad);
}
float ControlScheme::GetSignedInputForAction(GameAction negAction,
                                             GameAction posAction, 
                                             SDL_Gamepad *gamepad)
{
    return actions[posAction].GetValue(gamepad) 
         - actions[negAction].GetValue(gamepad);
}


void Input::Init()
{
    gDefaultControlScheme.actions[ACTION_STEER_LEFT].AddMapping(
            SDL_GAMEPAD_AXIS_LEFTX, false);
    gDefaultControlScheme.actions[ACTION_STEER_LEFT].AddMapping(
            SDL_SCANCODE_LEFT);
    gDefaultControlScheme.actions[ACTION_STEER_RIGHT].AddMapping(
            SDL_GAMEPAD_AXIS_LEFTX, true);
    gDefaultControlScheme.actions[ACTION_STEER_RIGHT].AddMapping(
            SDL_SCANCODE_RIGHT);

    gDefaultControlScheme.actions[ACTION_BRAKE].AddMapping(
            SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
    gDefaultControlScheme.actions[ACTION_BRAKE].AddMapping(
            SDL_SCANCODE_DOWN);

    gDefaultControlScheme.actions[ACTION_FORWARD].AddMapping(
            SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
    gDefaultControlScheme.actions[ACTION_FORWARD].AddMapping(
            SDL_SCANCODE_UP);

    gDefaultControlScheme.actions[ACTION_HANDBRAKE].AddMapping(
            SDL_GAMEPAD_BUTTON_SOUTH);
    gDefaultControlScheme.actions[ACTION_HANDBRAKE].AddMapping(
            SDL_SCANCODE_SPACE);

    
    gDefaultControlScheme.actions[ACTION_LOOK_LEFT].AddMapping(
            SDL_SCANCODE_A);
    gDefaultControlScheme.actions[ACTION_LOOK_LEFT].AddMapping(
            SDL_GAMEPAD_AXIS_RIGHTX, false);

    gDefaultControlScheme.actions[ACTION_LOOK_RIGHT].AddMapping(
            SDL_SCANCODE_D);
    gDefaultControlScheme.actions[ACTION_LOOK_RIGHT].AddMapping(
            SDL_GAMEPAD_AXIS_RIGHTX, true);
}


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
float Input::GetGamepadAxisPos(SDL_Gamepad *gamepad, SDL_GamepadAxis axis)
{
    return SDL_max(GetGamepadAxis(gamepad, axis), 0);
}
float Input::GetGamepadAxisNeg(SDL_Gamepad *gamepad, SDL_GamepadAxis axis)
{
    return -SDL_min(GetGamepadAxis(gamepad, axis), 0);
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

