#include <SDL3/SDL.h>

#include "physics.h"

#include "input.h"
#include "audio.h"
#include "model.h"
#include "convert.h"

#include <Jolt/Physics/Collision/BroadPhase/ObjectVsBroadPhaseLayerFilterTable.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayerInterfaceTable.h>
#include <Jolt/Physics/Collision/ObjectLayerPairFilterTable.h>
#include <Jolt/Physics/Vehicle/VehicleConstraint.h>
#include <Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
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
    
BodyID sphere_id;
BodyID sphere2_id;
BodyID sphereIds[NUM_SPHERES];
PhysicsSystem physics_system;
Body *floorBody;
Vec3 forwardDir;

Body *carBody;
Ref<VehicleConstraint> vehicleConstraint;
Ref<VehicleCollisionTester> colTester;
RefConst<Shape> carShape;
Ref<StaticCompoundShapeSettings> carCompoundShape;
// Car inputs
float carForward, carBrake, carSteer;
float carSteerTarget = 0;
float carDrivingDir = 1.0f; 

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


Vec3 sphereForce(0.0f, 0.0f, 0.0f);
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
    SDL_LogWarn(
            SDL_LOG_CATEGORY_ASSERT,
            "%s:%d: (%s) %s",
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
        (vehicleConstraint->GetController());
    VehicleEngine &engine = controller->GetEngine();
    float rpm = engine.GetCurrentRPM();
    SDL_SetAudioStreamFrequencyRatio(engineSnd->stream, rpm / 4000.0f);

    float averageSlip = 0;
    float slipLong;
    //Wheel *wheel1 = vehicleConstraint->GetWheels()[0];
    //SDL_Log("ang vel: %f", wheel1->GetAngularVelocity());
    for (Wheel *wheel : vehicleConstraint->GetWheels()) {
        if (!wheel->HasContact()) {
            continue;
        }
        Vec3 wPos = Vec3(carBody->GetWorldTransform() * Vec4(wheel->GetSettings()->mPosition, 1.0f));
        Vec3 wVel = carBody->GetPointVelocity(wPos);
        float wheelLongVel = wheel->GetAngularVelocity() * wheel->GetSettings()->mRadius;
        slipLong = SDL_fabsf(wheelLongVel) - SDL_fabsf(wVel.Dot(wheel->GetContactLongitudinal()));
        slipLong = SDL_fabsf(slipLong);
        //SDL_Log("%f", slipLong);

        float slipLat = wVel.Dot(wheel->GetContactLateral());
        float slip = slipLong*slipLong + slipLat*slipLat;
        averageSlip += slip / vehicleConstraint->GetWheels().size();
        //SDL_Log("slip long: %f, lat: %f", slipLong, slipLat);
    }
    //SDL_Log("slip long: %f", SDL_fabsf(slipLong));
    //SDL_Log("wheel velocity: %f", wVel.Length());
    //SDL_Log("Wheels: %d", vehicleConstraint->GetWheels().size());
    //SDL_Log("ang vel: %f", wheel->GetAngularVelocity());
    //SDL_Log("%f, %f, %f", wPos.GetX(), wPos.GetY(), wPos.GetZ());
    //SDL_Log("Slip average: %f", averageSlip);


    float driftGain = SDL_min(averageSlip / 130.0f, 1.0f);
    float driftPitch = carBody->GetLinearVelocity().Length() / 30.0f + 0.6;
    SDL_SetAudioStreamFrequencyRatio(driftSnd->stream, driftPitch);
    SDL_SetAudioStreamGain(driftSnd->stream, driftGain);
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
    carCompoundShape = new StaticCompoundShapeSettings;

    engineSnd = Audio::CreateSoundFromFile("sound/car_engine.wav");
    driftSnd = Audio::CreateSoundFromFile("sound/drift.wav");
    engineSnd->doRepeat = true;
    driftSnd->doRepeat = true;


    //engineSnd->play();

    isJoltSetup = true;
}


void Phys::AddCarWheel(Vec3 position, bool isSteering)
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


    carWheels.push_back(wheel);
}


void Phys::AddCarCollisionBox(Vec3 position, Vec3 scale)
{
    // Can only call this once at the moment
    SDL_Log("Adding collision box with size (%f, %f, %f) and pos (%f, %f, %f",
            scale.GetX(), scale.GetY(), scale.GetZ(),
            position.GetX(), position.GetY(), position.GetZ());
	Ref<Shape> shape = OffsetCenterOfMassShapeSettings(-position, new BoxShape(scale)).Create().Get();
    carCompoundShape->AddShape(position, Quat::sIdentity(), shape);
}



bool Phys::IsWheelFlipped(int wheelIndex)
{
    const WheelSettings *settings = vehicleConstraint->GetWheels()[wheelIndex]
                                                     ->GetSettings();
    return settings->mPosition.GetX() > 0.0f;
}


void Phys::CreateCarBody()
{
    /*
	const float wheel_radius = 0.3f;
	const float wheel_width = 0.1f;
	const float half_vehicle_length = 2.0f;
	const float half_vehicle_width = 0.9f;
	const float half_vehicle_height = 0.2f;
    const float max_steer_angle = SDL_PI_F / 6.0f;
    */

	BodyInterface &body_interface = physics_system.GetBodyInterface();

    // Create collision tester
	//colTester = new VehicleCollisionTesterCastSphere(Layers::MOVING, 0.5f * wheel_width);
	//colTester = new VehicleCollisionTesterCastCylinder(Layers::MOVING);
	colTester = new VehicleCollisionTesterRay(Layers::MOVING);
    
	// Create vehicle body
	RVec3 position(10, 3, 16);
	BodyCreationSettings car_body_settings(carCompoundShape, position, Quat::sRotation(Vec3::sAxisZ(), 0.0f), EMotionType::Dynamic, Layers::MOVING);
	car_body_settings.mOverrideMassProperties = EOverrideMassProperties::CalculateInertia;
	car_body_settings.mMassPropertiesOverride.mMass = 1500.0f;
	carBody = body_interface.CreateBody(car_body_settings);
	body_interface.AddBody(carBody->GetID(), EActivation::Activate);

	// Create vehicle constraint
	VehicleConstraintSettings vehicle;

    // Wheels
    /*
    // Left front
	WheelSettingsWV *w1 = new WheelSettingsWV;
	w1->mPosition = Vec3(half_vehicle_width, -0.9f * half_vehicle_height, half_vehicle_length - 2.0f * wheel_radius);
    w1->mMaxSteerAngle = max_steer_angle;

    // Right front
	WheelSettingsWV *w2 = new WheelSettingsWV;
	w2->mPosition = Vec3(-half_vehicle_width, -0.9f * half_vehicle_height, half_vehicle_length - 2.0f * wheel_radius);
    w2->mMaxSteerAngle = max_steer_angle;

    // Left rear
	WheelSettingsWV *w3 = new WheelSettingsWV;
	w3->mPosition = Vec3(half_vehicle_width, -0.9f * half_vehicle_height, -half_vehicle_length + 2.0f * wheel_radius);
    w3->mMaxSteerAngle = 0.0f;

    // Right rear
	WheelSettingsWV *w4 = new WheelSettingsWV;
	w4->mPosition = Vec3(-half_vehicle_width, -0.9f * half_vehicle_height, -half_vehicle_length + 2.0f * wheel_radius);
    w4->mMaxSteerAngle = 0.0f;
    */
    /*
    vehicle.mWheels = {w1, w2, w3, w4};
    
    for (WheelSettings *w : vehicle.mWheels) {
        w->mRadius = wheel_radius;
        w->mWidth = wheel_width;
    }
    */
    vehicle.mWheels = carWheels;

	WheeledVehicleControllerSettings *controller = new WheeledVehicleControllerSettings;
	vehicle.mController = controller;

	controller->mDifferentials.resize(1);
    for (unsigned int i = 0; i < carWheels.size(); i++) {
        Vec3 pos = carWheels[i]->mPosition;
        if (pos.GetX() < 0.0f && pos.GetZ() > 1.0f) {
            controller->mDifferentials[0].mLeftWheel = i;
        }
        else if (pos.GetX() > 1.0f && pos.GetZ() > 1.0f) {
            controller->mDifferentials[0].mRightWheel = i;
        }
    }
    

    controller->mTransmission.mMode = ETransmissionMode::Auto;
    SDL_Log("%f", controller->mTransmission.mShiftUpRPM);

	vehicleConstraint = new VehicleConstraint(*carBody, vehicle);
	physics_system.AddConstraint(vehicleConstraint);
	physics_system.AddStepListener(vehicleConstraint);



	vehicleConstraint->SetVehicleCollisionTester(colTester);
    vehicleConstraint->SetPostCollideCallback(VehiclePostCollideCallback);

    // TODO: remove and re-tweak car settings
	static_cast<WheeledVehicleController *>(vehicleConstraint->GetController())->SetTireMaxImpulseCallback(
		[](uint, float &outLongitudinalImpulse, float &outLateralImpulse, float inSuspensionImpulse, float inLongitudinalFriction, float inLateralFriction, float, float, float)
		{
			outLongitudinalImpulse = 10.0f * inLongitudinalFriction * inSuspensionImpulse;
			outLateralImpulse = inLateralFriction * inSuspensionImpulse;
		});

    //RVec3 com = Phys::GetCarPos();
    //SDL_Log("car com (%f, %f, %f)", com.GetX(), com.GetY(), com.GetZ());
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
	BodyInterface &body_interface = physics_system.GetBodyInterface();

    /*
	// Next we can create a rigid body to serve as the floor, we make a large box
	// Create the settings for the collision volume (the shape).
	// Note that for simple shapes (like boxes) you can also directly construct a BoxShape.
	BoxShapeSettings floor_shape_settings(Vec3(100.0f, 1.0f, 100.0f));
	floor_shape_settings.SetEmbedded(); // A ref counted object on the stack (base class RefTarget) should be marked as such to prevent it from being freed when its reference count goes to 0.

	// Create the shape
	ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
	ShapeRefC floor_shape = floor_shape_result.Get(); // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()


	// Create the settings for the body itself. Note that here you can also set other properties like the restitution / friction.
	BodyCreationSettings floor_settings(floor_shape, RVec3(0.0_r, -2.0_r, 0.0_r), Quat::sIdentity(), EMotionType::Static, Layers::NON_MOVING);

	// Create the actual rigid body
	floorBody = body_interface.CreateBody(floor_settings); // Note that if we run out of bodies this can return nullptr
    
    // Add it to the world
    body_interface.AddBody(floorBody->GetID(), EActivation::DontActivate);
    */



	// Now create a dynamic body to bounce on the floor
	// Note that this uses the shorthand version of creating and adding a body to the world
	BodyCreationSettings sphere_settings(new SphereShape(0.5f), RVec3(0.0_r, 2.0_r, 0.0_r), Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING);
    sphere_settings.mRestitution = 0.9f;
	sphere_id = body_interface.CreateAndAddBody(sphere_settings, EActivation::Activate);
	// Now you can interact with the dynamic body, in this case we're going to give it a velocity.
	// (note that if we had used CreateBody then we could have set the velocity straight on the body before adding it to the physics system)
	body_interface.SetLinearVelocity(sphere_id, Vec3(0.0f, -5.0f, 0.0f));

    // Second sphere
    //BodyCreationSettings sphere2_settings(new SphereShape(0.25f), RVec3(10.0_r, 1.0_r, 0.0_r), Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING);
    //sphere2_id = body_interface.CreateAndAddBody(sphere2_settings, EActivation::Activate);

    // Create all spheres
    for (size_t i = 0; i < NUM_SPHERES; i++) {
        BodyCreationSettings settings(new SphereShape(0.5f), RVec3(2*i + 1.0_r, 20.0_r, SDL_sin((float)i) * 3.0_r), Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING);
        sphereIds[i] = body_interface.CreateAndAddBody(settings, EActivation::Activate);
    }

    Phys::CreateCarBody();
    

	// Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance (it's pointless here because we only have 2 bodies).
	// You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
	// Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
	physics_system.OptimizeBroadPhase();
}


void Phys::PhysicsStep(float delta) 
{
    BodyInterface &body_interface = physics_system.GetBodyInterface();
    //body_interface.AddForce(sphere_id, sphereForce);
    //carSteer = carSteer + (carSteerTarget - carSteer) * 0.1f;
    if (!Input::GetGamepad()) {
        carSteer += glm::sign(carSteerTarget - carSteer) * 0.1f;
        carSteer = SDL_clamp(carSteer, -1.0f, 1.0f);
    } else {
        carSteer = carSteerTarget;
    }

    //RVec3 com = Phys::GetCarPos();
    //SDL_Log("car com (%f, %f, %f)", com.GetX(), com.GetY(), com.GetZ());

    if (carSteer || carForward || carBrake)
        body_interface.ActivateBody(carBody->GetID());

    WheeledVehicleController *controller = static_cast<WheeledVehicleController*>
        (vehicleConstraint->GetController());
    // Don't know what this does
    controller->SetDifferentialLimitedSlipRatio(1.4f);
    for (VehicleDifferentialSettings &d : controller->GetDifferentials())
        d.mLimitedSlipRatio = 1.4f;

    
    VehicleEngine &engine = controller->GetEngine();
    float rpm = engine.GetCurrentRPM();
    controller->GetTransmission().Update(delta, rpm, carForward, true);

    // Longitudinal velocity local to the car
    float longVelocity = (carBody->GetRotation().Conjugated() * carBody->GetLinearVelocity()).GetZ();

    if (longVelocity < 0.1f && carDrivingDir > 0.0f && carBrake > 0.0f) {
        // Braked to a stop when going forward, switch to going backward
        carDrivingDir = -1.0f;
    }
    else if (longVelocity > -0.1f && carDrivingDir < 0.0f && carForward > 0.0f) {
        // Braked to a stop when going backward, switch to going forward
        carDrivingDir = 1.0f;
    }

    if (carDrivingDir < 0.0f) {
        if (carBrake > 0.0f) {
            carForward = -carBrake;
            carBrake = 0.0f;
        }
        if (carForward > 0.0f) {
            carBrake = carForward;
            carForward = 0.0f;
        }
    }

    controller->SetDriverInput(carForward, carSteer, carBrake, 0.0f);

    // If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
    const int cCollisionSteps = 1;

    // Step the world
    physics_system.Update(delta, cCollisionSteps, &(*temp_allocator), &(*job_system));


}


void Phys::ProcessInput()
{
    sphereForce.SetX(0.0f);
    sphereForce.SetY(0.0f);
    sphereForce.SetZ(0.0f);
    //Vec3 rightDir = forwardDir.Cross(Vec3(0.0f, 1.0f, 0.0f));

    if (Input::GetGamepad()) {
        carForward = Input::GetGamepadAxis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
        carBrake = Input::GetGamepadAxis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
        carSteerTarget = Input::GetGamepadAxis(SDL_GAMEPAD_AXIS_LEFTX);
    }
    else {
        carSteerTarget = Input::GetScanAxis(SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT);
        carBrake = (float) Input::IsScanDown(SDL_SCANCODE_DOWN);
        carForward = (float) Input::IsScanDown(SDL_SCANCODE_UP);
    }

}


void Phys::SetForwardDir(Vec3 dir)
{
    dir.SetY(0.0f);
    dir = dir.Normalized();
    forwardDir = dir;
}


RMat44 Phys::GetWheelTransform(int wheelNum)
{
    //const WheelSettings *settings = vehicleConstraint->GetWheels()[wheelNum]
    //                                                 ->GetSettings();
    RMat44 wheelTransform = vehicleConstraint->GetWheelWorldTransform
                            (wheelNum, Vec3::sAxisY(), Vec3::sAxisX());
    return wheelTransform;
}


RVec3 Phys::GetCarPos() 
{
    BodyInterface &body_interface = physics_system.GetBodyInterface();
    return body_interface.GetCenterOfMassPosition(carBody->GetID());
}


Quat Phys::GetCarRotation()
{
    BodyInterface &body_interface = physics_system.GetBodyInterface();
    return body_interface.GetRotation(carBody->GetID());
}


RVec3 Phys::GetSpherePos()
{
    BodyInterface &body_interface = physics_system.GetBodyInterface();
    return body_interface.GetCenterOfMassPosition(sphere_id);
}


Quat Phys::GetSphereRotation()
{
    BodyInterface &body_interface = physics_system.GetBodyInterface();
    return body_interface.GetRotation(sphere_id);
}


RVec3 Phys::GetSpherePos(int sphereNum)
{
    BodyInterface &body_interface = physics_system.GetBodyInterface();
    return body_interface.GetCenterOfMassPosition(sphereIds[sphereNum]);
}


Quat Phys::GetSphereRotation(int sphereNum)
{
    BodyInterface &body_interface = physics_system.GetBodyInterface();
    return body_interface.GetRotation(sphereIds[sphereNum]);
}


void Phys::PhysicsCleanup()
{
	// Unregisters all types with the factory and cleans up the default material
    UnregisterTypes();

    BodyInterface &body_interface = physics_system.GetBodyInterface();

	// Remove the sphere from the physics system. Note that the sphere itself keeps all of its state and can be re-added at any time.
	body_interface.RemoveBody(sphere_id);

	// Destroy the sphere. After this the sphere ID is no longer valid.
	body_interface.DestroyBody(sphere_id);

    /*
    body_interface.RemoveBody(sphere2_id);
    body_interface.DestroyBody(sphere2_id);
    */

    // Destroy all spheres
    for (size_t i = 0; i < NUM_SPHERES; i++) {
        body_interface.RemoveBody(sphereIds[i]);
        body_interface.DestroyBody(sphereIds[i]);
    }

	// Remove and destroy the floor
	//body_interface.RemoveBody(floorBody->GetID());
	//body_interface.DestroyBody(floorBody->GetID());
    
	// Destroy the factory
	delete Factory::sInstance;
	Factory::sInstance = nullptr;
}






