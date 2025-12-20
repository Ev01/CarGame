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
    RACE_STARTED,
    RACE_ENDED
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
    void BeginRace();
    void EndRace(int winningPlayerIdx = -1);
    void Update(float delta);
    RaceState mState = RACE_NONE;
    float mCountdownTimer = 0.0;
    Uint64 mRaceStartMS = 0;
    float mTimePassed = 0.0;
    int mWinningPlayerIdx = -1;
    int mTotalLaps = 1;
};

struct ChoiceOption;
struct IntOption;

namespace World {
    void AssimpAddLight(const aiLight *aLight, const aiNode *aNode,
                        aiMatrix4x4 aTransform);

    void DestroyAllLights();
    void PrePhysicsUpdate(float delta);
    void OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2);
    void Update(float delta);
    void InputUpdate();
    void Init();
    void CleanUp();
    void CreateCars();
    void EndRace(int winningPlayerIdx = -1);
    void BeginRace();
    void BeginRaceCountdown();
    /*
    Vehicle& GetCar();
    Vehicle& GetCar2();
    */
    std::vector<Checkpoint>& GetCheckpoints();
    int GetNumCheckpointsPerLap();
    Model& GetCurrentMapModel();
    RaceState GetRaceState();
    RaceProgress& GetRaceProgress();

    extern ChoiceOption gMapOption;
    extern IntOption gLapsOption;
};
