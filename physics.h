#pragma once

#include "model.h"

// The Jolt headers don't include Jolt.h. Always include Jolt.h before including any other Jolt header.
// You can use Jolt.h in your precompiled header to speed up compilation.
#include <Jolt/Jolt.h>

// Jolt includes
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

#include <glm/glm.hpp>

#define NUM_SPHERES 20

namespace Phys {
    void SetupJolt();
    void SetupSimulation();
    void PhysicsStep(float delta);
    void CreateCarBody();
    void LoadMap(const Model &mapModel);

    JPH::RVec3 GetCarPos();
    JPH::Quat GetCarRotation();
    JPH::RMat44 GetWheelTransform(int wheelNum);
    void AddCarWheel(JPH::Vec3 position, bool isSteering);
    void AddCarCollisionBox(JPH::Vec3 position, JPH::Vec3 scale);
    bool IsWheelFlipped(int wheelIndex);

    JPH::RVec3 GetSpherePos();
    JPH::Quat GetSphereRotation();
    JPH::RVec3 GetSpherePos(int sphereNum);
    JPH::Quat GetSphereRotation(int sphereNum);
    /*
    JPH::RVec3 getSphere2Pos();
    JPH::Quat getSphere2Rotation();
    */
    void PhysicsCleanup();
    void ProcessInput();
    void SetForwardDir(JPH::Vec3 dir);
}

