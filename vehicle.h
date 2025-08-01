#pragma once

#include <Jolt/Jolt.h>

// Jolt includes
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Vehicle/VehicleConstraint.h>
#include <Jolt/RegisterTypes.h>

#include <string>



// Forward Declarations
struct Mix_Chunk;
struct Model;
namespace Audio {
    struct Sound;
};
namespace Render {
    struct SpotLight;
};

struct VehicleSettings
{
    void AddWheel(JPH::Vec3 position, bool isSteering, float wheelRadius, float wheelWidth);
    void AddCollisionBox(JPH::Vec3 position, JPH::Vec3 scale);
    // Loads the models
    void Init();

    std::string modelFile;
    std::string wheelModelFile;
    Model *vehicleModel;
    Model *wheelModel;
    float mass;
    float frontCamber;
    float frontToe;
    float frontCaster;
    float frontKingPin;
    float rearCamber;
    float rearToe;
    float rearCaster;
    float longGrip;
    float latGrip;
    float maxTorque;
    float suspensionMinLength = 0.1f;
    float suspensionMaxLength = 0.3f;
    float suspensionFrequency = 1.5f;
    float suspensionDamping = 0.5f;
    JPH::Ref<JPH::StaticCompoundShapeSettings> mCompoundShape;
    JPH::Array<JPH::Ref<JPH::WheelSettings>> mWheels;
    JPH::RMat44 headLightLeftTransform;
    JPH::RMat44 headLightRightTransform;
};

struct Vehicle
{
    bool IsWheelFlipped(int wheelIndex);
    void PrePhysicsUpdate(float delta);
    void Update();
    void Init(VehicleSettings &settings);
    void Destroy();

    // Puts on the handbrake and disables driving
    void HoldInPlace();
    void ReleaseFromHold();

    JPH::RMat44 GetWheelTransform(int wheelNum);
    JPH::RVec3 GetPos();
    JPH::Quat GetRotation();
    JPH::WheelSettings* GetWheelFR();
    JPH::WheelSettings* GetWheelFL();
    JPH::WheelSettings* GetWheelRR();
    JPH::WheelSettings* GetWheelRL();
    void DebugGUI(unsigned int id);

    float mForward = 0;
    float mBrake = 0;
    float mSteer = 0;
    float mSteerTarget = 0;
    float mHandbrake = 0;
    float mDrivingDir = 1.0f;
    bool mIsHeldInPlace = false;

    Audio::Sound *engineSnd;
    Audio::Sound *driftSnd;
    Render::SpotLight *headLightLeft;
    Render::SpotLight *headLightRight;
    JPH::RMat44 headLightLeftTransform;
    JPH::RMat44 headLightRightTransform;

    JPH::Body *mBody;

    // The below 4 could probably be removed and just have a pointer to
    // VehicleSettings object instead.
    Model *mVehicleModel;
    Model *mWheelModel;
    JPH::Ref<JPH::StaticCompoundShapeSettings> mCompoundShape;
    JPH::Array<JPH::Ref<JPH::WheelSettings>> mWheels;

    VehicleSettings *mSettings = nullptr;

    JPH::Ref<JPH::VehicleConstraint> mVehicleConstraint;
    JPH::Ref<JPH::VehicleCollisionTester> mColTester;
};

VehicleSettings GetVehicleSettingsFromFile(const char* filename);
const std::vector<Vehicle*>& GetExistingVehicles();
Vehicle* GetVehicleFromVehicleConstraint(const JPH::VehicleConstraint *constraint);
Vehicle* CreateVehicle();
void DestroyVehicle(Vehicle *toDestroy);

