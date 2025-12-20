#pragma once

#include <SDL3/SDL.h>

// Forward declares
//enum KeyStateFlag : unsigned int;

static constexpr int cMaxInputMappings = 3;

enum KeyStateFlag : unsigned int {
    KEY_STATE_FLAG_PRESSED       = 1,
    KEY_STATE_FLAG_JUST_PRESSED  = 2,
    KEY_STATE_FLAG_JUST_RELEASED = 4
};

enum MappingType {
    MAPPING_TYPE_NONE,
    MAPPING_TYPE_SCANCODE,
    MAPPING_TYPE_GAMEPAD_BUTTON,
    MAPPING_TYPE_GAMEPAD_AXIS_POS,
    MAPPING_TYPE_GAMEPAD_AXIS_NEG
};


// Each game action can have its own bindings. E.g. players will
// see these actions when changing their controls
enum GameAction : unsigned int {
    ACTION_STEER_RIGHT,
    ACTION_STEER_LEFT,
    ACTION_FORWARD,
    ACTION_BRAKE,
    ACTION_HANDBRAKE,
    ACTION_LOOK_LEFT,
    ACTION_LOOK_RIGHT,
    ACTION_UI_START,
    ACTION_UI_UP,
    ACTION_UI_DOWN,
    ACTION_UI_LEFT,
    ACTION_UI_RIGHT,
    ACTION_UI_ACCEPT,
    ACTION_PAUSE,
    ACTION_UI_CANCEL,
    

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
    float GetValue(SDL_Gamepad *gamepad = nullptr, KeyStateFlag flag=KEY_STATE_FLAG_PRESSED);
    float GetValue(int realKeyboardID = 0, KeyStateFlag flag=KEY_STATE_FLAG_PRESSED);
};


// Combines one or more input controls so that they can be used
// for a single action
struct InputAction {
    InputMapping mappings[cMaxInputMappings];
    // Index of last pressed input mapping
    //int lastPressed = 0;
    int numMappingsSet = 0;

    // Get value of last pressed mapping
    float GetValue(SDL_Gamepad *gamepad = nullptr, KeyStateFlag flag=KEY_STATE_FLAG_PRESSED);
    float GetValue(int realKeyboardID = 0, KeyStateFlag flag=KEY_STATE_FLAG_PRESSED);
    void AddMapping(SDL_Scancode scancode);
    void AddMapping(SDL_GamepadButton button);
    void AddMapping(SDL_GamepadAxis axis, bool isPosAxis = true);
    void ClearMappings();
    void HandleEvent(SDL_Event *event);
};


// Defines input bindings to every action in the game.
struct ControlScheme {
    InputAction actions[NUM_ACTIONS];

    // Gets the input value for an action between 0.0 and 1.0.
    float GetInputForAction(GameAction action, SDL_Gamepad *gamepad = nullptr, 
            KeyStateFlag flag=KEY_STATE_FLAG_PRESSED);
    float GetInputForAction(GameAction action, int realKeyboardID = 0, 
            KeyStateFlag flag=KEY_STATE_FLAG_PRESSED);
    // Like GetInputForAction, but can be used for two opposing actions to get a
    // value between -1.0 and 1.0. E.g. steer left and steer right.
    float GetSignedInputForAction(GameAction negAction, GameAction posAction, 
                                  SDL_Gamepad *gamepad = nullptr);
    float GetSignedInputForAction(GameAction negAction, GameAction posAction, 
                                  int realKeyboardID = 0);

    bool EventMatchesActionJustPressed(SDL_Event *event, GameAction action);
};

extern ControlScheme gDefaultControlScheme;
