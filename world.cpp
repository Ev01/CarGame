#include "world.h"
#include "render.h"
#include "convert.h"
#include "vehicle.h"
#include "model.h"
#include "physics.h"
#include "audio.h"

#include "vendor/imgui/imgui.h"

#include <assimp/matrix4x4.h>
#include <assimp/types.h>
#include <assimp/vector3.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

#include <glm/glm.hpp>
#include <SDL3/SDL.h>

#include <vector>
#include <algorithm>

static std::vector<Render::SpotLight*> spotLights;
static Vehicle *car;
static Vehicle *car2;
static VehicleSettings carSettings;
static VehicleSettings carSettings2;

static std::unique_ptr<Model> mapModel;
static glm::vec3 mapSpawnPoint = glm::vec3(0.0f);
static std::vector<Checkpoint> existingCheckpoints;
static Audio::Sound *checkpointSound;

RaceProgress gPlayerRaceProgress;
JPH::Vec3 raceStartPos;
JPH::Quat raceStartRot;

RaceState raceState;
static float raceCountdownTimer = 0.0;


static void CreateCheckpoint(JPH::Vec3 position, unsigned int num)
{
    Checkpoint &newCheckpoint = existingCheckpoints.emplace_back();
    newCheckpoint.Init(position);
    newCheckpoint.mNum = num;
}


static bool MapNodeCallback(const aiNode *node, const aiMatrix4x4 transform)
{
    aiQuaternion aRotation;
    aiVector3D aPosition;
    aiVector3D aScale;
    transform.Decompose(aScale, aRotation, aPosition);

    JPH::RMat44 joltTransform = ToJoltMat4(transform);
    JPH::Vec3 position = joltTransform.GetTranslation();

    if (SDL_strcmp(node->mName.C_Str(), "SpawnPoint") == 0) {
        mapSpawnPoint = ToGlmVec3(position);
    }
    else if (SDL_strncmp(node->mName.C_Str(), "checkpoint", 10) == 0) {
        /*
        Checkpoints should be named in the format checkpoint.### in the model 
        file, where ### is the number of the checkpoint and determines the order
        checkpoints should be taken.
        */
        // Pointer to the location of the . character in the node name
        char *dotLoc = SDL_strchr(node->mName.C_Str(), '.');
        char strNum[3];
        // Copy the 3 digits at the end to strNum and convert to integer
        SDL_strlcpy(strNum, dotLoc + 1, 4);
        unsigned int num = SDL_atoi(strNum);
        CreateCheckpoint(position, num);
        SDL_Log("Create checkpoint num = %d, strNum = %s", num, strNum);
    }
    else if (SDL_strcmp(node->mName.C_Str(), "RACE_START") == 0) {
        raceStartPos = ToJoltVec3(aPosition);
        raceStartRot = ToJoltQuat(aRotation);
    }

    return true;
}


static void LightCallback(const aiLight *aLight, const aiNode *aNode,
                           aiMatrix4x4 aTransform)
{
    // TODO: Make more effecient
    World::AssimpAddLight(aLight, aNode, aTransform);
    Render::AssimpAddLight(aLight, aNode, aTransform);
}


static void ChangeMap(const char *modelFileName)
{
    // Remove all checkpoints
    existingCheckpoints.clear();
    // Load the map
    JPH::BodyInterface &bodyInterface = Phys::GetBodyInterface();
    mapModel.reset(LoadModel(modelFileName, MapNodeCallback, LightCallback));
    Phys::LoadMap(*mapModel);
    // Sort checkpoints
    //std::sort(existingCheckpoints.begin(), existingCheckpoints.end(),
    //        [] (Checkpoint const& a, Checkpoint const& b) {
    // Respawn cars
    bodyInterface.SetPosition(World::GetCar().mBody->GetID(),
                              ToJoltVec3(mapSpawnPoint),
                              JPH::EActivation::Activate);
    bodyInterface.SetPosition(World::GetCar2().mBody->GetID(),
                              ToJoltVec3(mapSpawnPoint) + JPH::Vec3(6.0, 0, 0),
                              JPH::EActivation::Activate);
}


static void BeginRace()
{
    JPH::BodyInterface &bodyInterface = Phys::GetBodyInterface();
    bodyInterface.SetPositionRotationAndVelocity(World::GetCar().mBody->GetID(),
                                                 raceStartPos,
                                                 raceStartRot,
                                                 JPH::Vec3(0, 0, 0), JPH::Vec3(0, 0, 0));
    JPH::Vec3 p2Offset = raceStartRot.RotateAxisX() * -3.0;
    bodyInterface.SetPositionRotationAndVelocity(World::GetCar2().mBody->GetID(),
                                                 raceStartPos + p2Offset,
                                                 raceStartRot,
                                                 JPH::Vec3(0, 0, 0), JPH::Vec3(0, 0, 0));
    bodyInterface.ActivateBody(World::GetCar().mBody->GetID());
    bodyInterface.ActivateBody(World::GetCar2().mBody->GetID());
    raceState = RACE_COUNTING_DOWN;
    raceCountdownTimer = 3.0;
}

static void SEndRace()
{
    raceState = RACE_NONE;
}


/*
static void ResetCheckpoints()
{
    for (Checkpoint &c : existingCheckpoints) {
        c.mIsCollected = false;
    }
}
*/

void RaceProgress::EndRace()
{
    mFinishTime = (SDL_GetTicks() - mRaceStartMS) / 1000.0;
    SDL_Log("Finished race in %f seconds", mFinishTime);
}



void RaceProgress::CollectCheckpoint()
{
    mCheckpointsCollected++;
    //mCheckpointsCollected %= existingCheckpoints.size();
    if (mCheckpointsCollected == existingCheckpoints.size()) {
        SEndRace();
        EndRace();
    }
}


void RaceProgress::BeginRace()
{
    mRaceStartMS = SDL_GetTicks();
    mCheckpointsCollected = 0;
}


void Checkpoint::Init(JPH::Vec3 position)
{
    JPH::BoxShapeSettings shapeSettings(JPH::Vec3(2, 2, 2));
    JPH::ShapeRefC shape = shapeSettings.Create().Get();
    JPH::BodyCreationSettings bodySettings(shape, position,
                                           JPH::Quat::sIdentity(),
                                           JPH::EMotionType::Static,
                                           Phys::Layers::NON_SOLID);
    bodySettings.mIsSensor = true;
    JPH::BodyInterface &bodyInterface = Phys::GetBodyInterface();
    JPH::Body *checkpointBody = bodyInterface.CreateBody(bodySettings);
    bodyInterface.AddBody(checkpointBody->GetID(), JPH::EActivation::DontActivate);

    mBodyID = checkpointBody->GetID();
}


/*
void Checkpoint::Collect()
{
    mIsCollected = true;
}
*/


JPH::Vec3 Checkpoint::GetPosition() const
{
    return Phys::GetBodyInterface().GetPosition(mBodyID);
}


std::vector<Checkpoint>& World::GetCheckpoints()
{
    return existingCheckpoints;
}


void World::OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2)
{
    if (existingCheckpoints.size() == 0) return;
    if (raceState != RACE_STARTED) return;
    //JPH::BodyInterface &bodyInterface = Phys::GetPhysicsSystem().GetBodyInterfaceNoLock();
    //for (Checkpoint &checkpoint : existingCheckpoints) {

    Checkpoint &checkpoint = existingCheckpoints[gPlayerRaceProgress.mCheckpointsCollected % existingCheckpoints.size()];     
    if (inBody1.GetID() == checkpoint.mBodyID 
            || inBody2.GetID() == checkpoint.mBodyID) {
        //checkpoint.Collect();
        gPlayerRaceProgress.CollectCheckpoint();
        checkpointSound->Play();
        //if (existingCheckpoints.back().mIsCollected) {
        //    ResetCheckpoints();
        //}
        //break;
    }
    //}
}


void World::PrePhysicsUpdate(float delta)
{
    car->Update(delta);
    car2->Update(delta);

}


void World::Update(float delta)
{
    float raceTime;
    switch(raceState) {
        case RACE_COUNTING_DOWN:
            raceCountdownTimer -= delta;
            if (raceCountdownTimer < 0) {
                raceState = RACE_STARTED;
                gPlayerRaceProgress.BeginRace();
            }
            raceTime = -raceCountdownTimer;
            break;
        case RACE_STARTED:
            raceTime = (SDL_GetTicks() - gPlayerRaceProgress.mRaceStartMS) / 1000.0;
            break;
        case RACE_NONE:
            raceTime = gPlayerRaceProgress.mFinishTime;
            break;
    }

    car->PostPhysicsStep();
    car2->PostPhysicsStep();

    ImGui::Begin("Maps");
    const char* items[] = {"Map01", "Map02", "simple_map"};
    static int currentItem = 0;
    ImGui::Combo("Map", &currentItem, items, 3);
    if (ImGui::Button("Change map")) {
        World::DestroyAllLights();
        Render::DeleteAllLights();
        mapSpawnPoint = glm::vec3(0.0f);
        Phys::UnloadMap();
        switch(currentItem) {
            case 0:
                SDL_Log("sizeof: %lld", sizeof(*mapModel));
                ChangeMap("models/no_tex_map.gltf");
                break;
            case 1:
                ChangeMap("models/map1.gltf");
                break;
            case 2:
                ChangeMap("models/simple_map.gltf");
                break;
        }
    }
    if (ImGui::Button("Begin Race")) {
        BeginRace();
    }
    ImGui::Text("Race Time: %f", raceTime);
    ImGui::End();
    car->DebugGUI();
}


void World::ProcessInput()
{
    if (raceState != RACE_COUNTING_DOWN) {
        car->ProcessInput(false);
        car2->ProcessInput(true);
    }
    else {
        car->mHandbrake = 1.0f;
        car2->mHandbrake = 1.0f;
    }
}

void World::Init()
{
    // Audio
    checkpointSound = Audio::CreateSoundFromFile("sound/sound2.wav");
    checkpointSound->doRepeat = false;

    CreateCars();
    carSettings = GetVehicleSettingsFromFile("data/car.json");
    carSettings2 = GetVehicleSettingsFromFile("data/car2.json");
    carSettings.Init();
    carSettings2.Init();
    car->Init(carSettings);
    car2->Init(carSettings2);
    //CreateCheckpoint(JPH::Vec3(1, 2, 1));

    mapModel = std::unique_ptr<Model>(LoadModel("models/no_tex_map.gltf", MapNodeCallback, LightCallback));
    Phys::LoadMap(*mapModel);
    
    // Spawn second car away from first car
    Phys::GetBodyInterface().SetPosition(GetCar2().mBody->GetID(),
                                         JPH::Vec3(6.0, 0, 0),
                                         JPH::EActivation::Activate);
}


void World::CleanUp()
{
    DestroyVehicle(car);
    DestroyVehicle(car2);
}


void World::CreateCars()
{
    car = CreateVehicle();
    car2 = CreateVehicle();
}


Vehicle& World::GetCar()
{
    return *car;
}


Vehicle& World::GetCar2()
{
    return *car2;
}


void World::AssimpAddLight(const aiLight *aLight, const aiNode *aNode,
                           aiMatrix4x4 aTransform)
{
    aiQuaternion rotation;
    aiVector3D position;
    glm::mat4 glmTransform = ToGlmMat4(aTransform);
    switch (aLight->mType) {
        /*
        case aiLightSource_UNDEFINED:
            SDL_Log("Undefined light source!");
            break;

        case aiLightSource_POINT:
            SDL_Log("Adding Point light %s...", aLight->mName.C_Str());
            Light newLight;

            aTransform.DecomposeNoScaling(rotation, position);

            newLight.mPosition = ToGlmVec3(position);
            newLight.mColour = ToGlmVec3(aLight->mColorDiffuse);
            SDL_Log("Colour = (%f, %f, %f)", newLight.mColour.x, newLight.mColour.y, newLight.mColour.z);
            // NOTE: always 1 for gltf imports. Intensity would be stored in colour
            newLight.mQuadratic = aLight->mAttenuationQuadratic;

            lights.push_back(newLight);
            break;

        case aiLightSource_DIRECTIONAL:
            SDL_Log("Adding Sun Light...");
            //aTransform.DecomposeNoScaling(rotation, position);
            sunLight.mDirection = glm::vec3(glmTransform * glm::vec4(0.0, 0.0, -1.0, 0.0));
            sunLight.mColour = ToGlmVec3(aLight->mColorDiffuse);
            SDL_Log("Sun direction = %f, %f, %f", sunLight.mDirection.x, sunLight.mDirection.y, sunLight.mDirection.z);
            break;
        */

        case aiLightSource_SPOT:
        {
            SDL_Log("Adding Spot light %s...", aNode->mName.C_Str());
            Render::SpotLight *spotLight = Render::CreateSpotLight();

            aTransform.DecomposeNoScaling(rotation, position);
            glm::vec3 position2;
            position2 = glm::vec3(glmTransform * glm::vec4(ToGlmVec3(aLight->mPosition), 1.0));

            spotLight->mPosition = position2;
            glm::vec3 dir;
            dir = ToGlmVec3(aLight->mDirection);
            spotLight->mDirection = glm::vec3(glmTransform * glm::vec4(dir, 0.0));
            spotLight->mColour = ToGlmVec3(aLight->mColorDiffuse);
            spotLight->mQuadratic = aLight->mAttenuationQuadratic;
            spotLight->mCutoffInner = SDL_cos(aLight->mAngleInnerCone);
            spotLight->mCutoffOuter = SDL_cos(aLight->mAngleOuterCone);

            SDL_Log("Dir %f, %f, %f", spotLight->mDirection.x, spotLight->mDirection.y, spotLight->mDirection.z);
            SDL_Log("Col %f, %f, %f", spotLight->mColour.x, spotLight->mColour.y, spotLight->mColour.z);
            SDL_Log("Cutoff inner, outer: %f, %f", spotLight->mCutoffInner, spotLight->mCutoffOuter);
            SDL_Log("pos: %f, %f, %f", spotLight->mPosition.x, spotLight->mPosition.y, spotLight->mPosition.z);

            spotLights.push_back(spotLight);
            break;
        }

        default:
            break;
    }
}


void World::DestroyAllLights()
{
    for (Render::SpotLight *spotLight : spotLights) {
        Render::DestroySpotLight(spotLight);
    }
    spotLights.clear();
}


Model& World::GetCurrentMapModel() { return *mapModel; }
RaceState World::GetRaceState()           { return raceState; }
