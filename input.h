#pragma once

#include <SDL3/SDL.h>

#include <unordered_set>


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
    float GetValue(int realKeyboardID = 0);
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
    float GetValue(int realKeyboardID = 0);
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
    float GetInputForAction(GameAction action, SDL_Gamepad *gamepad = nullptr);
    float GetInputForAction(GameAction action, int realKeyboardID = 0);
    // Like GetInputForAction, but can be used for two opposing actions to get a
    // value between -1.0 and 1.0. E.g. steer left and steer right.
    float GetSignedInputForAction(GameAction negAction, GameAction posAction, 
                                  SDL_Gamepad *gamepad = nullptr);
    float GetSignedInputForAction(GameAction negAction, GameAction posAction, 
                                  int realKeyboardID = 0);
};

extern ControlScheme gDefaultControlScheme;


// Represents one physical keyboard. Some gaming keyboards are detected as
// multiple HID devices, each having one SDL_KeyboardID. This class groups them
// together if they belong to the same physical keyboard.
struct RealKeyboard {
    // Assigns a new id to this keyboard
    void Init();
    void Deinit();
    // Returns true if this SDL_KeyboardID is part of this RealKeyboard
    bool ContainsRawKeyboard(SDL_KeyboardID rawID);
    void HandleEvent(SDL_Event *event);
    bool IsScanDown(SDL_Scancode scancode);
    bool IsScanJustPressed(SDL_Scancode scancode);
    bool IsScanJustReleased(SDL_Scancode scancode);
    void NewFrame();
    // Return the name of the first SDL_Keyboard that belongs to this real
    // keyboard
    const char *GetName();
    int GetID()    { return id; }
    bool IsValid() { return id != 0; }

    Uint8 keyState[SDL_SCANCODE_COUNT];
    std::unordered_set<SDL_KeyboardID> rawKeyboardIDs;


    // Increments every time an id is assigned. Use this to assign a unique id.
    static int idCounter;
    // Get a pointer to the real keyboard given its id. Returns nullptr if the
    // id is invalid.
    static RealKeyboard* GetByID(int id);

private:
    int id = 0;
};

namespace Input
{
    void Init();
    bool IsScanDown(SDL_Scancode scancode);
    bool IsScanDown(SDL_Scancode scancode, SDL_KeyboardID keyboardID);
    bool IsScanJustReleased(SDL_Scancode scancode, SDL_KeyboardID keyboardID);
    bool IsScanJustPressed(SDL_Scancode scancode, SDL_KeyboardID keyboardID);
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
    int GetNumRealKeyboards();
    void HandleEvent(SDL_Event *event);
    SDL_Gamepad* GetGamepad();
    void DebugGUI();
    void Update();
    void NewFrame();
}
