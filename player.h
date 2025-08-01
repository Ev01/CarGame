#pragma once

#include "camera.h"
#include "input.h"
#include <SDL3/SDL.h>

#define NUM_PLAYERS 2

// Forward declarations:
struct Vehicle;
struct VehicleSettings;
//struct VehicleCamera;

struct Player {
    SDL_Gamepad *gamepad = nullptr;

    // The vehicle that this player controls
    Vehicle *vehicle = nullptr;
    VehicleCamera cam;

    // Create a vehicle and set it as the vehicle this player controls
    void CreateAndUseVehicle(VehicleSettings &settings);
    void SetVehicle(Vehicle *aVehicle);
    void InputUpdate();
    void PhysicsUpdate(double delta);
    // Must be called before using the player's camera
    void Init();
    //SDL_Gamepad* GetGamepad() { return gamepad; }
};

extern Player gPlayers[NUM_PLAYERS];
