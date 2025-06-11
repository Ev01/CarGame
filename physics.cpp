#include <SDL3/SDL.h>

#include "physics.h"

#include "input.h"
#include "audio.h"
#include "model.h"
#include "convert.h"
#include "vehicle.h"

#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayerInterfaceTable.h>
#include <Jolt/Physics/Collision/BroadPhase/ObjectVsBroadPhaseLayerFilterTable.h>
#include <Jolt/Physics/Collision/ObjectLayerPairFilterTable.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
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

Vehicle *car;
Vehicle *car2;

std::optional<BroadPhaseLayerInterfaceTable> broad_phase_layer_interface = std::nullopt;
std::optional<ObjectLayerPairFilterTable> object_vs_object_layer_filter = std::nullopt;
std::optional<ObjectVsBroadPhaseLayerFilterTable> object_vs_broadphase_layer_filter = std::nullopt;

std::optional<TempAllocatorImpl> temp_allocator = std::nullopt;
std::optional<JobSystemThreadPool> job_system = std::nullopt;

std::vector<BodyID> mapBodyIds;


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

void* reallocImpl(void *inBlock, size_t inOldSize, size_t inNewSize) 
{
    return SDL_realloc(inBlock, inNewSize);
}

void* alignedAllocateImpl(size_t inSize, size_t inAlignment) 
{
    return SDL_aligned_alloc(inAlignment, inSize);
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


void Phys::LoadMap(const Model &mapModel)
{
    BodyInterface &bodyInterface = physics_system.GetBodyInterface();
    for (size_t n = 0; n < mapModel.nodes.size(); n++) {
        const ModelNode &node = *(mapModel.nodes[n]);
        RMat44 transform = ToJoltMat4(node.mTransform);
        for (int meshIdx : node.mMeshes) {
            
            IndexedTriangleList triangleList;
            VertexList meshVertices;
            const Mesh &mesh = *(mapModel.meshes[meshIdx]);
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
            mapBodyIds.push_back(body->GetID());
        }
    }
    SDL_assert(mapBodyIds.size() > 0);
    BodyInterface::AddState state = bodyInterface.AddBodiesPrepare(
            mapBodyIds.data(), mapBodyIds.size());

    bodyInterface.AddBodiesFinalize(
            mapBodyIds.data(), mapBodyIds.size(),
            state, EActivation::DontActivate);
}


void Phys::UnloadMap()
{
    SDL_assert(mapBodyIds.size() > 0); // Map was never loaded
    BodyInterface &bodyInterface = physics_system.GetBodyInterface();
    bodyInterface.RemoveBodies(mapBodyIds.data(), mapBodyIds.size());
    bodyInterface.DestroyBodies(mapBodyIds.data(), mapBodyIds.size());
    mapBodyIds.clear();
}



bool Phys::CastRay(JPH::Vec3 start, JPH::Vec3 direction, JPH::Vec3 &outPos, const JPH::BodyFilter &inBodyFilter)
{
    JPH::RRayCast ray {start, direction};
    JPH::RayCastResult hit;

    bool hadHit = physics_system.GetNarrowPhaseQuery().CastRay(ray, hit, {}, {}, inBodyFilter);
    outPos = ray.GetPointOnRay(hit.mFraction);
    return hadHit;
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
    //car->Init();
    

	// Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance (it's pointless here because we only have 2 bodies).
	// You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
	// Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
	physics_system.OptimizeBroadPhase();
}


void Phys::PhysicsStep(float delta) 
{
    car->Update(delta);
    car2->Update(delta);

    // If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
    const int cCollisionSteps = 1;

    // Step the world
    physics_system.Update(delta, cCollisionSteps, &(*temp_allocator), &(*job_system));


}


void Phys::ProcessInput()
{
    car->ProcessInput();
    car2->ProcessInput();
}


void Phys::SetForwardDir(Vec3 dir)
{
    dir.SetY(0.0f);
    dir = dir.Normalized();
    forwardDir = dir;
}


void Phys::CleanUp()
{
	// Unregisters all types with the factory and cleans up the default material
    UnregisterTypes();

    DestroyVehicle(car);
    DestroyVehicle(car2);

	// Destroy the factory
	delete Factory::sInstance;
	Factory::sInstance = nullptr;
}


void Phys::CreateCars()
{
    car = CreateVehicle();
    car2 = CreateVehicle();
}


Vehicle& Phys::GetCar()
{
    return *car;
}

Vehicle& Phys::GetCar2()
{
    return *car2;
}


JPH::PhysicsSystem& Phys::GetPhysicsSystem()
{
    return physics_system;
}

JPH::BodyInterface&  Phys::GetBodyInterface()
{
    return physics_system.GetBodyInterface();
}

