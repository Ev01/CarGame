#include "player.h"

#include "vehicle.h"
#include "input_mapping.h"
#include "camera.h"
#include "render.h"
#include "world.h"

#include "../vendor/imgui/imgui.h"

Player gPlayers[MAX_PLAYERS];
int gNumPlayers = 0;

void PlayerRaceProgress::CollectCheckpoint()
{
    checkpointsCollected++;
    //mCheckpointsCollected %= existingCheckpoints.size();
    //if (checkpointsCollected == World::GetCheckpoints().size()) {
    //    World::EndRace();
    //}
}



bool PlayerRaceProgress::IsFinishedRace(int totalCheckpointCount)
{
    return checkpointsCollected >= totalCheckpointCount;
}


void PlayerRaceProgress::Reset()
{
    checkpointsCollected = 0;
}


int PlayerRaceProgress::GetLapsCompleted()
{
    int checkpointsPerLap = World::GetNumCheckpointsPerLap();
    if (checkpointsPerLap == 0) {
        return -1;
    } else {
        return checkpointsCollected / checkpointsPerLap;
    }
}


void Player::CreateAndUseVehicle(VehicleSettings &settings)
{
    Vehicle *v = CreateVehicle();
    v->Init(settings);
    SetVehicle(v);
}


void Player::InputUpdate()
{
    if (vehicle == nullptr) {
        return;
    }
    vehicle->mForward = GetInputForAction(ACTION_FORWARD);
    vehicle->mSteerTarget = GetSignedInputForAction(ACTION_STEER_LEFT,
                                                    ACTION_STEER_RIGHT);
    vehicle->mHandbrake = GetInputForAction(ACTION_HANDBRAKE);
    vehicle->mBrake = GetInputForAction(ACTION_BRAKE);
}


void Player::SetVehicle(Vehicle *aVehicle)
{
    vehicle = aVehicle;
    cam.targetBody = vehicle->mBody;
}


void Player::PhysicsUpdate(double delta)
{
    // Increase FOV based on speed.
    // Longitudinal velocity local to the car
    float longVelocity = vehicle->GetLongVelocity();
    float fovMult = SDL_clamp(longVelocity / camSettings.fovSpeedStrength + 1.0, 1.0, 2.0);
    float newFov = SDL_clamp(camSettings.baseFov * fovMult,
                             glm::radians(10.0), glm::radians(170.0));
    cam.cam.fov  = newFov;
    cam.cam.CalcProjection();

    // Update the player's camera
    float yawOffset;
    yawOffset = SDL_PI_F / 2.0 
                  * GetSignedInputForAction(ACTION_LOOK_LEFT, ACTION_LOOK_RIGHT);
    cam.SetFollowSmooth(yawOffset, camSettings.pitch, camSettings.dist,
                        camSettings.angleSmooth * delta, camSettings.distSmooth * delta,
                        camSettings.lift);
    

    // Interpolate steering if using keyboard
    if (inputDeviceType == INPUT_DEVICE_KEYBOARD && vehicle != nullptr) {
        float difference = glm::sign(vehicle->mSteerTarget - vehicle->mSteer)
                            * 0.1f;
        if (SDL_fabsf(difference) 
                > SDL_fabsf(vehicle->mSteerTarget - vehicle->mSteer)) {
            difference = vehicle->mSteerTarget - vehicle->mSteer;
        }
        vehicle->mSteer += difference;
        vehicle->mSteer = SDL_clamp(vehicle->mSteer, -1.0f, 1.0f);
    } else if (vehicle != nullptr) {
        vehicle->mSteer = vehicle->mSteerTarget;
    }
    


}

void Player::Init()
{
    cam.Init(glm::radians(95.0), Render::ScreenAspect(), 0.1f, 1000.0f);
    controlScheme = &gDefaultControlScheme;
    //keyboardMapping = &Input::gDefaultKeyboardMapping;
}


void Player::UseGamepad(SDL_Gamepad *aGamepad)
{
    inputDeviceType = INPUT_DEVICE_GAMEPAD;
    gamepad = aGamepad;
}


/*
void Player::UseKeyboard(SDL_KeyboardID aKeyboardID)
{
    inputDeviceType = INPUT_DEVICE_KEYBAORD;
    keyboardID = aKeyboardID;
}
*/
void Player::UseKeyboard(int aRealKeyboardID)
{
    inputDeviceType = INPUT_DEVICE_KEYBOARD;
    realKeyboardID = aRealKeyboardID;
}


bool Player::IsUsingGamepad(SDL_Gamepad *aGamepad)
{
    return inputDeviceType == INPUT_DEVICE_GAMEPAD && gamepad == aGamepad;
}
/*
bool Player::IsUsingKeyboard(SDL_KeyboardID aKeyboardID)
{
    return inputDeviceType == INPUT_DEVICE_KEYBAORD && keyboardID == aKeyboardID;
}
*/
bool Player::IsUsingKeyboard(int aRealKeyboardID)
{
    return inputDeviceType == INPUT_DEVICE_KEYBOARD 
        && realKeyboardID == aRealKeyboardID;
}
float Player::GetInputForAction(GameAction action)
{
    switch (inputDeviceType) {
        case INPUT_DEVICE_KEYBOARD:
            return controlScheme->GetInputForAction(action, realKeyboardID);
        case INPUT_DEVICE_GAMEPAD:
            return controlScheme->GetInputForAction(action, gamepad);
        case INPUT_DEVICE_NONE:
            /*
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, 
                        "Tried to get input but player does not have an assigned"
                        "input device.");
            */
            break;
    }
    return 0.0;
}

float Player::GetSignedInputForAction(GameAction negAction, GameAction posAction)
{
    switch (inputDeviceType) {
        case INPUT_DEVICE_KEYBOARD:
            return controlScheme->GetSignedInputForAction(
                    negAction, posAction, realKeyboardID);
        case INPUT_DEVICE_GAMEPAD:
            return controlScheme->GetSignedInputForAction(negAction, posAction,
                                                          gamepad);
        case INPUT_DEVICE_NONE:
            /*
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, 
                        "Tried to get input but player does not have an assigned"
                        "input device.");
            */
            break;
    }
    return 0.0;
}


int Player::AddPlayer()
{
    gPlayers[gNumPlayers].Init();
    return gNumPlayers++;
}


void Player::RemovePlayer(int playerIdx)
{
    if (playerIdx >= gNumPlayers) return;
    // Shift players to the right in array down
    for (int i = playerIdx; i < gNumPlayers - 1; i++) {
        gPlayers[i] = gPlayers[i + 1];
    }
    gNumPlayers--;
}

void Player::AddPlayerAndVehicle(VehicleSettings &settings)
{
    Player::AddPlayer();
    gPlayers[gNumPlayers - 1].CreateAndUseVehicle(settings);
}


void Player::PhysicsUpdateAllPlayers(double delta)
{
    for (int i = 0; i < gNumPlayers; i++) {
        gPlayers[i].PhysicsUpdate(delta);
    }
}


void Player::InputUpdateAllPlayers()
{
    for (int i = 0; i < gNumPlayers; i++) {
        gPlayers[i].InputUpdate();
    }
}


void Player::DebugGUI() 
{
    ImGui::Begin("Camera Settings", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);
    float fovDegrees = glm::degrees(camSettings.baseFov);
    ImGui::SliderFloat("FOV", &fovDegrees, 20.0f, 140.0f);
    //cam.cam.SetFovAndRecalcProjection(glm::radians(fovDegrees));
    camSettings.baseFov = glm::radians(fovDegrees);
    ImGui::SliderFloat("Camera Pitch", &camSettings.pitch,
                       -SDL_PI_F / 2.0f, SDL_PI_F / 2.0);
    ImGui::SliderFloat("Camera Distance", &camSettings.dist, 0.0f, 30.0f);
    ImGui::SliderFloat("Camera Angle Smoothing", &camSettings.angleSmooth, 0.0f, 20.0f);
    ImGui::SliderFloat("Camera Distance Smoothing", &camSettings.distSmooth, 0.0f, 20.0f);
    ImGui::SliderFloat("Camera Lift", &camSettings.lift, 0.0f, 20.0f);
    ImGui::SliderFloat("FOV Speed Stength", &camSettings.fovSpeedStrength, 25.0f, 500.0f);
    ImGui::End();
}


void Player::ResetVehiclePointers()
{
    for (int i = 0; i < gNumPlayers; i++) {
        gPlayers[i].vehicle = nullptr;
    }
}
