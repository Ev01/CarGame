#include <SDL3/SDL.h>

#include "physics.h"

#include "input.h"
#include "audio.h"
#include "model.h"
#include "convert.h"

#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayerInterfaceTable.h>
#include <Jolt/Physics/Collision/BroadPhase/ObjectVsBroadPhaseLayerFilterTable.h>
#include <Jolt/Physics/Collision/ObjectLayerPairFilterTable.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Vehicle/VehicleConstraint.h>
#include <Jolt/Physics/Vehicle/WheeledVehicleController.h>

#include <SDL3/SDL.h>

#include <glm/gtc/quaternion.hpp>

// STL includes
#include <iostream>
#include <cstdarg>
#include <thread>
#include <optional>

#define SPHERE_FORCE_MAG 2000.0f

// TODO: remove these
using namespace JPH;
using namespace JPH::literals;

bool isJoltSetup = false;
    
PhysicsSystem physics_system;
Body *floorBody;
Vec3 forwardDir;

Vehicle car;

std::optional<BroadPhaseLayerInterfaceTable> broad_phase_layer_interface = std::nullopt;
std::optional<ObjectLayerPairFilterTable> object_vs_object_layer_filter = std::nullopt;
std::optional<ObjectVsBroadPhaseLayerFilterTable> object_vs_broadphase_layer_filter = std::nullopt;

std::optional<TempAllocatorImpl> temp_allocator = std::nullopt;
std::optional<JobSystemThreadPool> job_system = std::nullopt;

// Audio
Audio::Sound *engineSnd;
Audio::Sound *driftSnd;

// Layer that objects can be in, determines which other objects it can collide with
// Typically you at least want to have 1 layer for moving bodies and 1 layer for static bodies, but you can have more
// layers if you want. E.g. you could have a layer for high detail collision (which is not used by the physics simulation
// but only if you do collision testing).
namespace Layers
{
	static constexpr ObjectLayer NON_MOVING = 0;
	static constexpr ObjectLayer MOVING = 1;
	static constexpr ObjectLayer NUM_LAYERS = 2;
};


// Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
// a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
// You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case) but if you have
// many object layers you'll be creating many broad phase trees, which is not efficient. If you want to fine tune
// your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.
namespace BroadPhaseLayers
{
	static constexpr BroadPhaseLayer NON_MOVING(0);
	static constexpr BroadPhaseLayer MOVING(1);
	static constexpr uint NUM_LAYERS(2);
};


Array<Ref<WheelSettings>> carWheels;


// Callback for traces, connect this to your own trace function if you have one
static void traceImpl(const char *inFMT, ...)
{
	// Format the message
	va_list list;
	va_start(list, inFMT);
	char buffer[1024];
	SDL_vsnprintf(buffer, sizeof(buffer), inFMT, list);
	va_end(list);

	// Print to the TTY
	SDL_Log(buffer);
}


// Callback for asserts, connect this to your own assert handler if you have one
static bool assertFailedImpl(const char *inExpression, const char *inMessage, const char *inFile, uint inLine)
{
	// Print to the TTY
	// cout << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr? inMessage : "") << endl;
    SDL_Log(
           // SDL_LOG_CATEGORY_ASSERT,
            "JPH ASSERT FAILED: %s:%d: (%s) %s",
            inFile,
            inLine,
            inExpression,
            inMessage != nullptr ? inMessage : (char*)""
    );

	// Breakpoint
	return true;
};

void* reallocImpl(void *inBlock, size_t inOldSize, size_t inNewSize) {
    return SDL_realloc(inBlock, inNewSize);
}

void* alignedAllocateImpl(size_t inSize, size_t inAlignment) {
    return SDL_aligned_alloc(inAlignment, inSize);
}


static void VehiclePostCollideCallback(VehicleConstraint &inVehicle, const PhysicsStepListenerContext &inContext)
{
    // Audio stuff
    WheeledVehicleController *controller = static_cast<WheeledVehicleController*>
        (inVehicle.GetController());
    VehicleEngine &engine = controller->GetEngine();
    float rpm = engine.GetCurrentRPM();
    SDL_SetAudioStreamFrequencyRatio(car.engineSnd->stream, rpm / 4000.0f);

    float averageSlip = 0;
    float slipLong;
    //Wheel *wheel1 = vehicleConstraint->GetWheels()[0];
    //SDL_Log("ang vel: %f", wheel1->GetAngularVelocity());
    for (Wheel *wheel : inVehicle.GetWheels()) {
        if (!wheel->HasContact()) {
            continue;
        }
        Vec3 wPos = Vec3(car.mBody->GetWorldTransform() * Vec4(wheel->GetSettings()->mPosition, 1.0f));
        Vec3 wVel = car.mBody->GetPointVelocity(wPos);
        float wheelLongVel = wheel->GetAngularVelocity() * wheel->GetSettings()->mRadius;
        slipLong = SDL_fabsf(wheelLongVel) - SDL_fabsf(wVel.Dot(wheel->GetContactLongitudinal()));
        slipLong = SDL_fabsf(slipLong);
        //SDL_Log("%f", slipLong);

        float slipLat = wVel.Dot(wheel->GetContactLateral());
        float slip = slipLong*slipLong + slipLat*slipLat;
        averageSlip += slip / inVehicle.GetWheels().size();
        //SDL_Log("slip long: %f, lat: %f", slipLong, slipLat);
    }
    //SDL_Log("slip long: %f", SDL_fabsf(slipLong));
    //SDL_Log("wheel velocity: %f", wVel.Length());
    //SDL_Log("Wheels: %d", vehicleConstraint->GetWheels().size());
    //SDL_Log("ang vel: %f", wheel->GetAngularVelocity());
    //SDL_Log("%f, %f, %f", wPos.GetX(), wPos.GetY(), wPos.GetZ());
    //SDL_Log("Slip average: %f", averageSlip);


    float driftGain = SDL_min(averageSlip / 130.0f, 1.0f);
    float driftPitch = car.mBody->GetLinearVelocity().Length() / 30.0f + 0.6;
    SDL_SetAudioStreamFrequencyRatio(car.driftSnd->stream, driftPitch);
    SDL_SetAudioStreamGain(car.driftSnd->stream, driftGain);
}


void Phys::SetupJolt() 
{
	// Register allocation hook. In this example we'll just let Jolt use malloc / free but you can override these if you want (see Memory.h).
	// This needs to be done before any other Jolt function is called.
	//RegisterDefaultAllocator();
    // Use SDL allocate functions for memory handling.
    Allocate = SDL_malloc;
    Reallocate = reallocImpl;
    Free = SDL_free;
    AlignedAllocate = alignedAllocateImpl;
    AlignedFree = SDL_aligned_free;

	// Install trace and assert callbacks
	Trace = traceImpl;
	JPH_IF_ENABLE_ASSERTS(AssertFailed = assertFailedImpl;)

	// Create a factory, this class is responsible for creating instances of classes based on their name or hash and is mainly used for deserialization of saved data.
	// It is not directly used in this example but still required.
	Factory::sInstance = new Factory();

    // We need a temp allocator for temporary allocations during the physics update. We're
    // pre-allocating 10 MB to avoid having to do allocations during the physics update.
    // B.t.w. 10 MB is way too much for this example but it is a typical value you can use.
    // If you don't want to pre-allocate you can also use TempAllocatorMalloc to fall back to
    // malloc / free.
    temp_allocator.emplace(10 * 1024 * 1024);

    // We need a job system that will execute physics jobs on multiple threads. Typically
    // you would implement the JobSystem interface yourself and let Jolt Physics run on top
    // of your own job scheduler. JobSystemThreadPool is an example implementation.
    job_system.emplace(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);

	// Register all physics types with the factory and install their collision handlers with the CollisionDispatch class.
	// If you have your own custom shape types you probably need to register their handlers with the CollisionDispatch before calling this function.
	// If you implement your own default material (PhysicsMaterial::sDefault) make sure to initialize it before this function or else this function will create one for you.
	RegisterTypes();



    //engineSnd->play();

    isJoltSetup = true;
}


void Vehicle::AddWheel(Vec3 position, bool isSteering)
{
    SDL_Log("Add Wheel with position %f, %f, %f", 
            position.GetX(), position.GetY(), position.GetZ());
	const float wheel_radius = 0.45f;
	const float wheel_width = 0.2f;
	WheelSettingsWV *wheel = new WheelSettingsWV;
	wheel->mPosition = position;
    wheel->mMaxSteerAngle = isSteering ? SDL_PI_F / 5.0 : 0;
    wheel->mRadius = wheel_radius;
    wheel->mWidth = wheel_width;
    wheel->mSuspensionMinLength = 0.1f;
    wheel->mSuspensionMaxLength = 0.3f;

    //wheel->mSuspensionSpring.mFrequency = 0.5f;
    //wheel->mSuspensionSpring.mDamping = 1.0f;


    mWheels.push_back(wheel);
}


void Vehicle::AddCollisionBox(Vec3 position, Vec3 scale)
{

    SDL_Log("Adding collision box with size (%f, %f, %f) and pos (%f, %f, %f",
            scale.GetX(), scale.GetY(), scale.GetZ(),
            position.GetX(), position.GetY(), position.GetZ());
    if (!mCompoundShape) {
        mCompoundShape = new StaticCompoundShapeSettings;
    }

	Ref<Shape> shape = OffsetCenterOfMassShapeSettings(-position, new BoxShape(scale)).Create().Get();
    mCompoundShape->AddShape(position, Quat::sIdentity(), shape);
}

bool Vehicle::IsWheelFlipped(int wheelIndex)
{
    const WheelSettings *settings = mVehicleConstraint->GetWheels()[wheelIndex]
                                                     ->GetSettings();
    return settings->mPosition.GetX() > 0.0f;
}


void Vehicle::Init()
{
	BodyInterface &bodyInterface = physics_system.GetBodyInterface();

    // Create collision tester
	//colTester = new VehicleCollisionTesterCastSphere(Layers::MOVING, 0.5f * wheel_width);
	//colTester = new VehicleCollisionTesterCastCylinder(Layers::MOVING);
	mColTester = new VehicleCollisionTesterRay(Layers::MOVING);
    
	// Create vehicle body
	RVec3 position(6, 3, 12);
	BodyCreationSettings carBodySettings(mCompoundShape, position, Quat::sRotation(Vec3::sAxisZ(), 0.0f), EMotionType::Dynamic, Layers::MOVING);
	carBodySettings.mOverrideMassProperties = EOverrideMassProperties::CalculateInertia;
	carBodySettings.mMassPropertiesOverride.mMass = 1500.0f;
	mBody = bodyInterface.CreateBody(carBodySettings);
	bodyInterface.AddBody(mBody->GetID(), EActivation::Activate);

	// Create vehicle constraint
	VehicleConstraintSettings constraintSettings;

    constraintSettings.mWheels = mWheels;

	WheeledVehicleControllerSettings *controller = new WheeledVehicleControllerSettings;
	constraintSettings.mController = controller;

	controller->mDifferentials.resize(1);
    for (unsigned int i = 0; i < mWheels.size(); i++) {
        Vec3 pos = mWheels[i]->mPosition;
        if (pos.GetX() < 0.0f && pos.GetZ() > 1.0f) {
            controller->mDifferentials[0].mLeftWheel = i;
        }
        else if (pos.GetX() > 1.0f && pos.GetZ() > 1.0f) {
            controller->mDifferentials[0].mRightWheel = i;
        }
    }
    

    controller->mTransmission.mMode = ETransmissionMode::Auto;
    SDL_Log("%f", controller->mTransmission.mShiftUpRPM);

	mVehicleConstraint = new VehicleConstraint(*mBody, constraintSettings);
	physics_system.AddConstraint(mVehicleConstraint);
	physics_system.AddStepListener(mVehicleConstraint);



	mVehicleConstraint->SetVehicleCollisionTester(mColTester);
    mVehicleConstraint->SetPostCollideCallback(VehiclePostCollideCallback);

    // TODO: remove and re-tweak car settings
	static_cast<WheeledVehicleController *>(mVehicleConstraint->GetController())->SetTireMaxImpulseCallback(
		[](uint, float &outLongitudinalImpulse, float &outLateralImpulse, float inSuspensionImpulse, float inLongitudinalFriction, float inLateralFriction, float, float, float)
		{
			outLongitudinalImpulse = 10.0f * inLongitudinalFriction * inSuspensionImpulse;
			outLateralImpulse = inLateralFriction * inSuspensionImpulse;
		});

    //RVec3 com = Phys::GetCarPos();
    //SDL_Log("car com (%f, %f, %f)", com.GetX(), com.GetY(), com.GetZ());
    engineSnd = Audio::CreateSoundFromFile("sound/car_engine.wav");
    driftSnd = Audio::CreateSoundFromFile("sound/drift.wav");
    engineSnd->doRepeat = true;
    driftSnd->doRepeat = true;
}


void Phys::LoadMap(const Model &mapModel)
{
    std::vector<BodyID> bodyIds;
    BodyInterface &bodyInterface = physics_system.GetBodyInterface();
    for (const ModelNode &node : mapModel.nodes) {
        RMat44 transform = ToJoltMat4(node.mTransform);
        for (int meshIdx : node.mMeshes) {
            IndexedTriangleList triangleList;
            VertexList meshVertices;
            const Mesh &mesh = mapModel.meshes[meshIdx];
            for (size_t i = 0; i < mesh.vertices.size(); i++) {
                Vec3 vertexV3 = ToJoltVec3(mesh.vertices[i].position);
                vertexV3 = Vec3(transform * Vec4(vertexV3, 1.0f));
                Float3 vertexF3(vertexV3.GetX(), vertexV3.GetY(), vertexV3.GetZ());
                meshVertices.push_back(vertexF3);
            }
            
            for (size_t i = 0; i < mesh.indices.size(); i += 3) {
                int i1 = mesh.indices[i];
                int i2 = mesh.indices[i + 1];
                int i3 = mesh.indices[i + 2];
                IndexedTriangle triangle(i1, i2, i3, 0);
                triangleList.push_back(triangle);
            }
            Body *body = bodyInterface.CreateBody(
                    BodyCreationSettings(
                        new MeshShapeSettings(meshVertices, triangleList),
                        RVec3::sZero(),
                        Quat::sIdentity(),
                        EMotionType::Static,
                        Layers::NON_MOVING));
            bodyIds.push_back(body->GetID());
        }
    }
    SDL_assert(bodyIds.size() > 0);
    BodyInterface::AddState state = bodyInterface.AddBodiesPrepare(
            &bodyIds[0], bodyIds.size());

    bodyInterface.AddBodiesFinalize(
            &bodyIds[0], bodyIds.size(),
            state, EActivation::DontActivate);
}


void Phys::SetupSimulation()
{
    if (!isJoltSetup) Phys::SetupJolt();

	// This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
	// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
	const uint cMaxBodies = 1024;

	// This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
	const uint cNumBodyMutexes = 0;
    
	// This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
	// body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
	// too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
	// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
	const uint cMaxBodyPairs = 1024;

	// This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
	// number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
	// Note: This value is low because this is a simple test. For a real project use something in the order of 10240.
	const uint cMaxContactConstraints = 1024;
    

    // Create mapping table from object layer to broadphase layer
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
    broad_phase_layer_interface.emplace(
            Layers::NUM_LAYERS,
            BroadPhaseLayers::NUM_LAYERS
            );

    broad_phase_layer_interface->MapObjectToBroadPhaseLayer(
            Layers::NON_MOVING, BroadPhaseLayers::NON_MOVING);
    broad_phase_layer_interface->MapObjectToBroadPhaseLayer(
            Layers::MOVING, BroadPhaseLayers::MOVING);

    // Create class that filters object vs object layers
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
    object_vs_object_layer_filter.emplace(Layers::NUM_LAYERS);

    object_vs_object_layer_filter->EnableCollision
            (Layers::MOVING, Layers::MOVING);
    object_vs_object_layer_filter->EnableCollision
            (Layers::MOVING, Layers::NON_MOVING);

    // Create class that filters object vs broadphase layers
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
    object_vs_broadphase_layer_filter.emplace
            (*broad_phase_layer_interface, BroadPhaseLayers::NUM_LAYERS,
             *object_vs_object_layer_filter, Layers::NUM_LAYERS);

                                                    

	// Now we can create the actual physics system.
	physics_system.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, *broad_phase_layer_interface, *object_vs_broadphase_layer_filter, *object_vs_object_layer_filter);

	// The main way to interact with the bodies in the physics system is through the body interface. There is a locking and a non-locking
	// variant of this. We're going to use the locking version (even though we're not planning to access bodies from multiple threads)
	//BodyInterface &body_interface = physics_system.GetBodyInterface();

    //Phys::CreateCarBody();
    car.Init();
    

	// Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance (it's pointless here because we only have 2 bodies).
	// You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
	// Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
	physics_system.OptimizeBroadPhase();
}


void Vehicle::Update(float delta)
{
    BodyInterface &bodyInterface = physics_system.GetBodyInterface();
    //mSteer = mSteer + (mSteerTarget - mSteer) * 0.1f;
    if (!Input::GetGamepad()) {
        mSteer += glm::sign(mSteerTarget - mSteer) * 0.1f;
        mSteer = SDL_clamp(mSteer, -1.0f, 1.0f);
    } else {
        mSteer = mSteerTarget;
    }

    //RVec3 com = Phys::GetCarPos();
    //SDL_Log("car com (%f, %f, %f)", com.GetX(), com.GetY(), com.GetZ());

    if (mSteer || mForward || mBrake)
        bodyInterface.ActivateBody(mBody->GetID());

    WheeledVehicleController *controller = static_cast<WheeledVehicleController*>
        (mVehicleConstraint->GetController());
    // Don't know what this does
    controller->SetDifferentialLimitedSlipRatio(1.4f);
    for (VehicleDifferentialSettings &d : controller->GetDifferentials())
        d.mLimitedSlipRatio = 1.4f;

    
    VehicleEngine &engine = controller->GetEngine();
    float rpm = engine.GetCurrentRPM();
    controller->GetTransmission().Update(delta, rpm, mForward, true);

    // Longitudinal velocity local to the car
    float longVelocity = (mBody->GetRotation().Conjugated() * mBody->GetLinearVelocity()).GetZ();

    if (longVelocity < 0.1f && mDrivingDir > 0.0f && mBrake > 0.0f) {
        // Braked to a stop when going forward, switch to going backward
        mDrivingDir = -1.0f;
    }
    else if (longVelocity > -0.1f && mDrivingDir < 0.0f && mForward > 0.0f) {
        // Braked to a stop when going backward, switch to going forward
        mDrivingDir = 1.0f;
    }

    if (mDrivingDir < 0.0f) {
        if (mBrake > 0.0f) {
            mForward = -mBrake;
            mBrake = 0.0f;
        }
        if (mForward > 0.0f) {
            mBrake = mForward;
            mForward = 0.0f;
        }
    }

    controller->SetDriverInput(mForward, mSteer, mBrake, 0.0f);
}


void Phys::PhysicsStep(float delta) 
{
    car.Update(delta);

    // If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
    const int cCollisionSteps = 1;

    // Step the world
    physics_system.Update(delta, cCollisionSteps, &(*temp_allocator), &(*job_system));


}


void Vehicle::ProcessInput()
{
    if (Input::GetGamepad()) {
        mForward = Input::GetGamepadAxis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
        mBrake = Input::GetGamepadAxis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
        mSteerTarget = Input::GetGamepadAxis(SDL_GAMEPAD_AXIS_LEFTX);
    }
    else {
        mSteerTarget = Input::GetScanAxis(SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT);
        mBrake = (float) Input::IsScanDown(SDL_SCANCODE_DOWN);
        mForward = (float) Input::IsScanDown(SDL_SCANCODE_UP);
    }
}


void Phys::ProcessInput()
{
    car.ProcessInput();
}


void Phys::SetForwardDir(Vec3 dir)
{
    dir.SetY(0.0f);
    dir = dir.Normalized();
    forwardDir = dir;
}


RMat44 Vehicle::GetWheelTransform(int wheelNum)
{
    RMat44 wheelTransform = mVehicleConstraint->GetWheelWorldTransform
                            (wheelNum, Vec3::sAxisY(), Vec3::sAxisX());
    return wheelTransform;
}


JPH::RVec3 Vehicle::GetPos()
{
    BodyInterface &bodyInterface = physics_system.GetBodyInterface();
    return bodyInterface.GetCenterOfMassPosition(mBody->GetID());
}


JPH::Quat Vehicle::GetRotation()
{
    BodyInterface &bodyInterface = physics_system.GetBodyInterface();
    return bodyInterface.GetRotation(mBody->GetID());
}


void Vehicle::Destroy()
{
    BodyInterface &bodyInterface = physics_system.GetBodyInterface();

    bodyInterface.RemoveBody(mBody->GetID());
    bodyInterface.DestroyBody(mBody->GetID());
}


void Phys::PhysicsCleanup()
{
	// Unregisters all types with the factory and cleans up the default material
    UnregisterTypes();

    car.Destroy();

	// Destroy the factory
	delete Factory::sInstance;
	Factory::sInstance = nullptr;
}


Vehicle& Phys::GetCar()
{
    return car;
}




