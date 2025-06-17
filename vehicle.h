#pragma once

#include <Jolt/Jolt.h>

// Jolt includes
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Vehicle/VehicleConstraint.h>
#include <Jolt/RegisterTypes.h>




// Forward Declarations
struct Mix_Chunk;
namespace Audio {
    struct Sound;
};
namespace Render {
    struct SpotLight;
};

struct VehicleSettings
{
    void AddWheel(JPH::Vec3 position, bool isSteering);
    void AddCollisionBox(JPH::Vec3 position, JPH::Vec3 scale);

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
    JPH::Ref<JPH::StaticCompoundShapeSettings> mCompoundShape;
    JPH::Array<JPH::Ref<JPH::WheelSettings>> mWheels;
    JPH::RMat44 headLightLeftTransform;
    JPH::RMat44 headLightRightTransform;
};

struct Vehicle
{
    void AddWheel(JPH::Vec3 position, bool isSteering);
    void AddCollisionBox(JPH::Vec3 position, JPH::Vec3 scale);
    bool IsWheelFlipped(int wheelIndex);
    void Update(float delta);
    void ProcessInput(bool useGamepad=false);
    void Init(VehicleSettings &settings);
    void Destroy();

    float mForward = 0;
    float mBrake = 0;
    float mSteer = 0;
    float mSteerTarget = 0;
    float mHandbrake = 0;
    float mDrivingDir = 1.0f;

    JPH::RMat44 GetWheelTransform(int wheelNum);
    JPH::RVec3 GetPos();
    JPH::Quat GetRotation();
    JPH::WheelSettings* GetWheelFR();
    JPH::WheelSettings* GetWheelFL();
    JPH::WheelSettings* GetWheelRR();
    JPH::WheelSettings* GetWheelRL();

    Audio::Sound *engineSnd;
    Audio::Sound *driftSnd;
    Render::SpotLight *headLightLeft;
    Render::SpotLight *headLightRight;
    JPH::RMat44 headLightLeftTransform;
    JPH::RMat44 headLightRightTransform;

    JPH::Body *mBody;
    JPH::Ref<JPH::StaticCompoundShapeSettings> mCompoundShape;
    JPH::Array<JPH::Ref<JPH::WheelSettings>> mWheels;
    JPH::Ref<JPH::VehicleConstraint> mVehicleConstraint;
    JPH::Ref<JPH::VehicleCollisionTester> mColTester;
};

VehicleSettings GetVehicleSettingsFromFile(const char* filename);
Vehicle* GetVehicleFromVehicleConstraint(const JPH::VehicleConstraint *constraint);
Vehicle* CreateVehicle();
void DestroyVehicle(Vehicle *toDestroy);

