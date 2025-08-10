#include "player.h"

#include "vehicle.h"
#include "input.h"
#include "camera.h"
#include "render.h"

Player gPlayers[MAX_PLAYERS];
int gNumPlayers = 0;

// Camera Settings
static float camPitch = -0.4f;
static float camDist = 3.7f;
static float angleSmooth = 3.5f;
static float distSmooth = 17.0f;

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
    float yawOffset;
    yawOffset = SDL_PI_F / 2.0 
                  * GetSignedInputForAction(ACTION_LOOK_LEFT, ACTION_LOOK_RIGHT);
    cam.SetFollowSmooth(yawOffset, camPitch, camDist,
                        angleSmooth * delta, distSmooth * delta);
    

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


void Player::AddPlayer()
{
    gPlayers[gNumPlayers].Init();
    gNumPlayers++;
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
