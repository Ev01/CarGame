#include "input_mapping.h"
#include "input.h"

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


float InputMapping::GetValue(SDL_Gamepad *gamepad, KeyStateFlag flag)
{
    /*
    if (gamepad == nullptr && mappingType != MAPPING_TYPE_SCANCODE) {
        return 0.0;
    }
    */

    switch (mappingType) {
        case MAPPING_TYPE_SCANCODE:
            break;
        case MAPPING_TYPE_GAMEPAD_AXIS_POS:
            return Input::GetGamepadAxisPos(gamepad, axis);
        case MAPPING_TYPE_GAMEPAD_AXIS_NEG:
            return Input::GetGamepadAxisNeg(gamepad, axis);
        case MAPPING_TYPE_GAMEPAD_BUTTON:
            switch (flag) {
                case KEY_STATE_FLAG_PRESSED:
                    return Input::GetGamepadButton(gamepad, button);
                case KEY_STATE_FLAG_JUST_PRESSED:
                    return Input::GetGamepadButtonJustPressed(gamepad, button);
                case KEY_STATE_FLAG_JUST_RELEASED:
                    return Input::GetGamepadButtonJustReleased(gamepad, button);
            }
        case MAPPING_TYPE_NONE:
            return 0.0;
    }

    return 0.0;
}


float InputMapping::GetValue(int realKeyboardID, KeyStateFlag flag)
{
    //if (realKeyboardID == 0 || mappingType != MAPPING_TYPE_SCANCODE) {
    if (mappingType != MAPPING_TYPE_SCANCODE) {
        return 0.0;
    }
    if (realKeyboardID == 0) {
        // If the keyboard ID is invalid, default to SDL's internal keyboard
        // state, which does not differentiate between different keyboards.
        return (float) SDL_GetKeyboardState(NULL)[scancode];
    }
    
    RealKeyboard *realKeyboard = RealKeyboard::GetByID(realKeyboardID);
    if (realKeyboard != nullptr) {
        switch (flag) {
            case KEY_STATE_FLAG_PRESSED:
                return (float) realKeyboard->IsScanDown(scancode);
            case KEY_STATE_FLAG_JUST_PRESSED:
                return (float) realKeyboard->IsScanJustPressed(scancode);
            case KEY_STATE_FLAG_JUST_RELEASED:
                return (float) realKeyboard->IsScanJustReleased(scancode);
        }
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


float InputAction::GetValue(SDL_Gamepad *gamepad, KeyStateFlag flag) 
{
    float toReturn = 0.0;
    for (int i = 0; i < numMappingsSet; i++) {
        // Choose the mapping with the greatest input value. This should
        // work fine in most occasions
        toReturn = SDL_max(toReturn, mappings[i].GetValue(gamepad, flag));
    }
    return toReturn;
}


float InputAction::GetValue(int realKeyboardID, KeyStateFlag flag) 
{
    /*
    if (realKeyboardID == 0) {
        return 0.0;
    }
    */
    float toReturn = 0.0;
    for (int i = 0; i < numMappingsSet; i++) {
        // Choose the mapping with the greatest input value. This should
        // work fine in most occasions
        toReturn = SDL_max(toReturn, mappings[i].GetValue(realKeyboardID, flag));
    }
    return toReturn;
}


void InputAction::ClearMappings()
{
    numMappingsSet = 0;
}


float ControlScheme::GetInputForAction(GameAction action, SDL_Gamepad *gamepad, KeyStateFlag flag)
{
    return actions[action].GetValue(gamepad, flag);
}


float ControlScheme::GetInputForAction(GameAction action,
                                       int realKeyboardID, KeyStateFlag flag)
{
    return actions[action].GetValue(realKeyboardID, flag);
}


float ControlScheme::GetSignedInputForAction(GameAction negAction,
                                             GameAction posAction, 
                                             SDL_Gamepad *gamepad)
{
    return actions[posAction].GetValue(gamepad) 
         - actions[negAction].GetValue(gamepad);
}


float ControlScheme::GetSignedInputForAction(GameAction negAction,
                                             GameAction posAction, 
                                             int realKeyboardID)
{
    return actions[posAction].GetValue(realKeyboardID) 
         - actions[negAction].GetValue(realKeyboardID);
}


bool ControlScheme::EventMatchesActionJustPressed(SDL_Event *event, GameAction action)
{
    for (int i = 0; i < cMaxInputMappings; i++) {
        InputMapping &mapping = actions[action].mappings[i];
        if (mapping.mappingType == MAPPING_TYPE_SCANCODE && event->type == SDL_EVENT_KEY_DOWN
                && mapping.scancode == event->key.scancode) {
            return true;
        }
        if (mapping.mappingType == MAPPING_TYPE_GAMEPAD_BUTTON 
                && event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN
                && event->gbutton.button == mapping.button) {
            return true;
        }
    }
    return false;
}
