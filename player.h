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
    void ProcessInput();
    void PhysicsUpdate(double delta);
    void Init();
    //SDL_Gamepad* GetGamepad() { return gamepad; }
};

extern Player gPlayers[NUM_PLAYERS];
