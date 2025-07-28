#include "player.h"

#include "vehicle.h"
#include "input.h"
#include "camera.h"
#include "render.h"

Player gPlayers[NUM_PLAYERS];

// Camera Settings
static float camPitch = -0.4f;
static float camDist = 3.7f;
static float angleSmooth = 3.5f;
static float distSmooth = 17.0f;

void Player::CreateAndUseVehicle(VehicleSettings &settings)
{
    SetVehicle(CreateVehicle());
    vehicle->Init(settings);
}


void Player::ProcessInput()
{
    if (vehicle == nullptr) {
        return;
    }
    if (gamepad != nullptr) {
        vehicle->mForward     = Input::GetGamepadAxis(
                                    gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
        vehicle->mBrake       = Input::GetGamepadAxis(
                                    gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
        vehicle->mSteerTarget = Input::GetGamepadAxis(
                                    gamepad, SDL_GAMEPAD_AXIS_LEFTX);
        vehicle->mHandbrake   = (float) Input::GetGamepadButton(
                                            gamepad, SDL_GAMEPAD_BUTTON_SOUTH);
    }
    else {
        vehicle->mSteerTarget = Input::GetScanAxis(SDL_SCANCODE_LEFT,
                                                   SDL_SCANCODE_RIGHT);
        vehicle->mBrake = (float) Input::IsScanDown(SDL_SCANCODE_DOWN);
        vehicle->mForward = (float) Input::IsScanDown(SDL_SCANCODE_UP);
        vehicle->mHandbrake = (float) Input::IsScanDown(SDL_SCANCODE_SPACE);
    }
}


void Player::SetVehicle(Vehicle *aVehicle)
{
    vehicle = aVehicle;
    cam.targetBody = vehicle->mBody;
}


void Player::PhysicsUpdate(double delta)
{
    float yawOffset;
    if (gamepad != nullptr) {
        yawOffset = SDL_PI_F / 2.0f 
                  * Input::GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTX);
    } else {
        yawOffset = SDL_PI_F / 2.0f 
                  * Input::GetScanAxis(SDL_SCANCODE_D, SDL_SCANCODE_A);
    }
    cam.SetFollowSmooth(yawOffset, camPitch, camDist,
                        angleSmooth * delta, distSmooth * delta);
}

void Player::Init()
{
    cam.Init(glm::radians(95.0), Render::ScreenAspect(), 0.1f, 1000.0f);
}
