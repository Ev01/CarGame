#pragma once

#include "model.h"

// The Jolt headers don't include Jolt.h. Always include Jolt.h before including any other Jolt header.
// You can use Jolt.h in your precompiled header to speed up compilation.
#include <Jolt/Jolt.h>

// Jolt includes
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Vehicle/VehicleConstraint.h>
#include <Jolt/RegisterTypes.h>

#include <glm/glm.hpp>

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


namespace Phys {
    void SetupJolt();
    void SetupSimulation();
    void PhysicsStep(float delta);
    void LoadMap(const Model &mapModel);

    void PhysicsCleanup();
    void ProcessInput();
    void SetForwardDir(JPH::Vec3 dir);
    Vehicle& GetCar();
}



