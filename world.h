#pragma once

#include <assimp/defs.h>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <SDL3/SDL.h>

// Forward declarations
struct aiLight;
struct aiNode;
struct Vehicle;
struct Model;

template<typename TReal> class aiMatrix4x4t;
typedef aiMatrix4x4t<ai_real> aiMatrix4x4;
namespace JPH {
    struct Body;
    struct Vec3;
};

enum RaceState {
    RACE_NONE,
    RACE_COUNTING_DOWN,
    RACE_STARTED
};

struct Checkpoint {
    void Init(JPH::Vec3 position);
    //void Collect();
    JPH::Vec3 GetPosition() const;
    //void SetPosition(JPH::Vec3 newPos);

    //bool mIsCollected = false;
    unsigned short int mNum = 0;
    JPH::BodyID mBodyID;
};


struct RaceProgress {
    void CollectCheckpoint();
    void BeginRace();
    void EndRace();
    Uint64 mRaceStartMS = 0;
    unsigned int mCheckpointsCollected = 0;
    float mFinishTime = 0.0;
};
extern RaceProgress gPlayerRaceProgress;


namespace World {
    void AssimpAddLight(const aiLight *aLight, const aiNode *aNode,
                        aiMatrix4x4 aTransform);

    void DestroyAllLights();
    void PrePhysicsUpdate(float delta);
    void OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2);
    void Update(float delta);
    void ProcessInput();
    void Init();
    void CleanUp();
    void CreateCars();
    Vehicle& GetCar();
    Vehicle& GetCar2();
    std::vector<Checkpoint>& GetCheckpoints();
    Model& GetCurrentMapModel();
    RaceState GetRaceState();
};
