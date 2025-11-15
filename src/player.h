#pragma once

#include "camera.h"
#include <SDL3/SDL.h>

#define MAX_PLAYERS 4

// Forward declarations:
struct Vehicle;
struct VehicleSettings;
struct ControlScheme;
enum GameAction : unsigned int;
//struct VehicleCamera;

enum InputDeviceType {
    INPUT_DEVICE_NONE,
    INPUT_DEVICE_GAMEPAD,
    INPUT_DEVICE_KEYBOARD,
};

struct Player {
    // Whether the player is using a gamepad or keyboard
    InputDeviceType inputDeviceType = INPUT_DEVICE_NONE;
    SDL_Gamepad *gamepad = nullptr;
    SDL_KeyboardID keyboardID = 0;
    int realKeyboardID = 0;
    ControlScheme *controlScheme = nullptr;

    // The vehicle that this player controls
    Vehicle *vehicle = nullptr;
    VehicleCamera cam;

    // Create a vehicle and set it as the vehicle this player controls
    void CreateAndUseVehicle(VehicleSettings &settings);
    // Must call Init on vehicle before using this function
    void SetVehicle(Vehicle *aVehicle);
    void InputUpdate();
    void PhysicsUpdate(double delta);
    // Must be called before using the player's camera
    void Init();
    //SDL_Gamepad* GetGamepad() { return gamepad; }
    void UseGamepad(SDL_Gamepad *aGamepad);
    //void UseKeyboard(SDL_KeyboardID aKeyboardID);
    void UseKeyboard(int aRealKeyboardID);
    bool IsUsingGamepad(SDL_Gamepad *aGamepad);
    //bool IsUsingKeyboard(SDL_KeyboardID aKeyboardID);
    bool IsUsingKeyboard(int aRealKeyboardID);
    float GetInputForAction(GameAction action);
    float GetSignedInputForAction(GameAction negAction, GameAction posAction);

    static void AddPlayer();
    static void AddPlayerAndVehicle(VehicleSettings &settings);
    static void PhysicsUpdateAllPlayers(double delta);
    static void InputUpdateAllPlayers();
};

extern Player gPlayers[MAX_PLAYERS];
extern int gNumPlayers;
