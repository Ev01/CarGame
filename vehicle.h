#pragma once

#include <Jolt/Jolt.h>

// Jolt includes
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Vehicle/VehicleConstraint.h>
#include <Jolt/RegisterTypes.h>


// Forward Declarations
namespace Audio {
    struct Sound;
}

struct Vehicle
{
    void AddWheel(JPH::Vec3 position, bool isSteering);
    void AddCollisionBox(JPH::Vec3 position, JPH::Vec3 scale);
    bool IsWheelFlipped(int wheelIndex);
    void Update(float delta);
    void ProcessInput();
    void Init();
    void Destroy();

    float mForward, mBrake, mSteer, mSteerTarget;
    float mDrivingDir = 1.0f;

    JPH::RMat44 GetWheelTransform(int wheelNum);
    JPH::RVec3 GetPos();
    JPH::Quat GetRotation();

    Audio::Sound *engineSnd;
    Audio::Sound *driftSnd;
    JPH::Body *mBody;
    JPH::Ref<JPH::StaticCompoundShapeSettings> mCompoundShape;
    JPH::Array<JPH::Ref<JPH::WheelSettings>> mWheels;
    JPH::Ref<JPH::VehicleConstraint> mVehicleConstraint;
    JPH::Ref<JPH::VehicleCollisionTester> mColTester;
};
