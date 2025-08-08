#pragma once

#include "camera.h"
#include <SDL3/SDL.h>

#define MAX_PLAYERS 4

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
    // Must call Init on vehicle before using this function
    void SetVehicle(Vehicle *aVehicle);
    void InputUpdate();
    void PhysicsUpdate(double delta);
    // Must be called before using the player's camera
    void Init();
    //SDL_Gamepad* GetGamepad() { return gamepad; }
    

    static void AddPlayer();
    static void PhysicsUpdateAllPlayers(double delta);
    static void InputUpdateAllPlayers();
};

extern Player gPlayers[MAX_PLAYERS];
extern int gNumPlayers;
