#pragma once

#include <SDL3/SDL.h>



struct InputState {
    float carSteer = 0.0;
    float carForward = 0.0;
    float carBrake = 0.0;
    float carHandbrake = 0.0;
};

struct InputMappingKeyboard {
    SDL_Scancode carSteerRight = SDL_SCANCODE_RIGHT;
    SDL_Scancode carSteerLeft  = SDL_SCANCODE_LEFT;
    SDL_Scancode carForward    = SDL_SCANCODE_UP;
    SDL_Scancode carBrake      = SDL_SCANCODE_DOWN;
    SDL_Scancode carHandbrake  = SDL_SCANCODE_SPACE;
};

// Actions that one or more keys/buttons/axis can be bound to
//enum InputAction {
//
//};

enum MappingType {
    MAPPING_TYPE_NONE,
    MAPPING_TYPE_SCANCODE,
    MAPPING_TYPE_GAMEPAD_BUTTON,
    MAPPING_TYPE_GAMEPAD_AXIS_POS,
    MAPPING_TYPE_GAMEPAD_AXIS_NEG
};


// Each game action can have its own bindings. E.g. players will
// see these actions when changing their controls
enum GameAction {
    ACTION_STEER_RIGHT,
    ACTION_STEER_LEFT,
    ACTION_FORWARD,
    ACTION_BRAKE,
    ACTION_HANDBRAKE,
    ACTION_LOOK_LEFT,
    ACTION_LOOK_RIGHT,

    NUM_ACTIONS
};


// Defines a single input control. This can be a keyboard key, 
// controller button or controller axis.
struct InputMapping {
    MappingType mappingType = MAPPING_TYPE_NONE;
    union {
        SDL_Scancode scancode;
        SDL_GamepadButton button;
        SDL_GamepadAxis axis;
    };

    void AssignMapping(SDL_Scancode aScancode);
    void AssignMapping(SDL_GamepadButton aButton);
    void AssignMapping(SDL_GamepadAxis aAxis, bool isPosAxis = true);
    float GetValue(SDL_Gamepad *gamepad = nullptr);
};


// Combines one or more input controls so that they can be used
// for a single action
struct InputAction {
    InputMapping mappings[3];
    // Index of last pressed input mapping
    //int lastPressed = 0;
    int numMappingsSet = 0;

    // Get value of last pressed mapping
    float GetValue(SDL_Gamepad *gamepad = nullptr);
    void AddMapping(SDL_Scancode scancode);
    void AddMapping(SDL_GamepadButton button);
    void AddMapping(SDL_GamepadAxis axis, bool isPosAxis = true);
    void ClearMappings();
    void HandleEvent(SDL_Event *event);
};

// Defines input bindings to every action in the game.
struct ControlScheme {
    InputAction actions[NUM_ACTIONS];

    float GetInputForAction(GameAction action, SDL_Gamepad *gamepad = nullptr);
    float GetSignedInputForAction(GameAction negAction, GameAction posAction, 
                                  SDL_Gamepad *gamepad = nullptr);
};

extern ControlScheme gDefaultControlScheme;

namespace Input
{
    void Init();
    bool IsScanDown(SDL_Scancode scancode);
    // Return 1.0f if posScan is pressed, -1.0f if negScan is pressed, and 0.0f
    // if neither or both keys are pressed. Can be used for left/right or
    // up/down keys.
    float GetScanAxis(SDL_Scancode negScan, SDL_Scancode posScan);
    // Returns number between -1.0 and 1.0 for gamepad axis (e.g. left stick).
    // Will be 0.0 to 1.0 for triggers.
    float GetGamepadAxis(SDL_Gamepad *gamepad, SDL_GamepadAxis axis);
    // Returns the amount a joystick axis is in the positive direction, or zero
    // if it is in the negative direction.
    float GetGamepadAxisPos(SDL_Gamepad *gamepad, SDL_GamepadAxis axis);
    // Returns the amount a joystick axis is in the negative direction, or zero
    // if it is in the positive direction. Will always return a positive number
    float GetGamepadAxisNeg(SDL_Gamepad *gamepad, SDL_GamepadAxis axis);
    bool GetGamepadButton(SDL_Gamepad *gamepad, SDL_GamepadButton button);
    void HandleEvent(SDL_Event *event);
    SDL_Gamepad* GetGamepad();
    void DebugGUI();

    extern InputMappingKeyboard gDefaultKeyboardMapping;
}
