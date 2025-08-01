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


void Player::InputUpdate()
{
    if (vehicle == nullptr) {
        return;
    }
    vehicle->mForward = gDefaultControlScheme.GetInputForAction(
            ACTION_FORWARD, gamepad);
    vehicle->mSteerTarget = gDefaultControlScheme.GetSignedInputForAction(
            ACTION_STEER_LEFT, ACTION_STEER_RIGHT, gamepad);
    vehicle->mHandbrake = gDefaultControlScheme.GetInputForAction(
            ACTION_HANDBRAKE, gamepad);
    vehicle->mBrake = gDefaultControlScheme.GetInputForAction(
            ACTION_BRAKE, gamepad);
}


void Player::SetVehicle(Vehicle *aVehicle)
{
    vehicle = aVehicle;
    cam.targetBody = vehicle->mBody;
}


void Player::PhysicsUpdate(double delta)
{
    float yawOffset;
    yawOffset = SDL_PI_F / 2.0 
                  * gDefaultControlScheme.GetSignedInputForAction(
                        ACTION_LOOK_LEFT, ACTION_LOOK_RIGHT, gamepad);
    cam.SetFollowSmooth(yawOffset, camPitch, camDist,
                        angleSmooth * delta, distSmooth * delta);
    

    // Interpolate steering if using keyboard
    if (gamepad == nullptr && vehicle != nullptr) {
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
    //keyboardMapping = &Input::gDefaultKeyboardMapping;
}
