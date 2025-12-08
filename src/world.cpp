#include "world.h"
#include "render.h"
#include "render_lights.h"
#include "convert.h"
#include "vehicle.h"
#include "model.h"
#include "physics.h"
#include "audio.h"
#include "player.h"
#include "options.h"

#include "../vendor/imgui/imgui.h"

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

const char* mapDisplayNames[] = {"Map01", "Map02", "simple_map", "racetrack1"};
const char* mapFilepaths[] = {
    "data/models/no_tex_map.gltf",
    "data/models/map1.gltf",
    "data/models/simple_map.gltf",
    "data/models/racetrack1.gltf",
};

ChoiceOption World::gMapOption = {0, mapDisplayNames, 4};
    

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


static void RespawnVehicles()
{
    JPH::BodyInterface &bodyInterface = Phys::GetBodyInterface();
    for (int i = 0; i < Vehicle::NumExistingVehicles(); i++) {
        Vehicle *v = Vehicle::GetExistingVehicles()[i];
        bodyInterface.SetPosition(
                v->mBody->GetID(),
                ToJoltVec3(mapSpawnPoint) + JPH::Vec3(3*i, 0, 0),
                JPH::EActivation::Activate
        );
    }

}


static void ChangeMap(const char *modelFileName)
{
    World::DestroyAllLights();
    Render::DeleteAllLights();
    mapSpawnPoint = glm::vec3(0.0f);
    Phys::UnloadMap();
    // Remove all checkpoints
    existingCheckpoints.clear();
    // Reset shader spotlights to prevent phantom lights.
    Render::ResetSpotLightsGPU();
    // Load the map
    mapModel.reset(LoadModel(modelFileName, MapNodeCallback, LightCallback));
    Phys::LoadMap(*mapModel);
    // Sort checkpoints
    //std::sort(existingCheckpoints.begin(), existingCheckpoints.end(),
    //        [] (Checkpoint const& a, Checkpoint const& b) {
    // Respawn cars
    RespawnVehicles();
    
    SDL_Log("Map spot lights: %d", (int) spotLights.size());
}


static void BeginRace()
{
    JPH::BodyInterface &bodyInterface = Phys::GetBodyInterface();

    for (int i = 0; i < Vehicle::NumExistingVehicles(); i++) {
        JPH::Vec3 offset = raceStartRot.RotateAxisX() * -3.0 * i;
        bodyInterface.SetPositionRotationAndVelocity(
                Vehicle::GetExistingVehicles()[i]->mBody->GetID(),
                raceStartPos + offset,
                raceStartRot,
                JPH::Vec3(0, 0, 0), JPH::Vec3(0, 0, 0)
        );
        bodyInterface.ActivateBody(
                Vehicle::GetExistingVehicles()[i]->mBody->GetID());
    }

    /*
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
    */
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
    Vehicle::PrePhysicsUpdateAllVehicles(delta);
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

    Vehicle::UpdateAllVehicles();

    ImGui::Begin("Maps", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);

    //static int currentItem = 0;
    ImGui::Combo("Map", &gMapOption.selectedChoice, gMapOption.optionStrings, gMapOption.numOptions);
    if (ImGui::Button("Change map")) {
        ChangeMap(mapFilepaths[gMapOption.selectedChoice]);
    }
    if (ImGui::Button("Begin Race")) {
        BeginRace();
    }
    ImGui::Text("Race Time: %f", raceTime);
    ImGui::End();
    for (int i = 0; i < Vehicle::NumExistingVehicles(); i++) {
        Vehicle::GetExistingVehicles()[i]->DebugGUI(i);
    }
}


void World::InputUpdate()
{
    if (raceState != RACE_COUNTING_DOWN) {
        for (Vehicle *v : Vehicle::GetExistingVehicles()) {
            v->ReleaseFromHold();
        }
    }
    else {
        for (Vehicle *v : Vehicle::GetExistingVehicles()) {
            v->HoldInPlace();
        }
    }
}

void World::Init()
{
    // Audio
    checkpointSound = Audio::CreateSoundFromFile("data/sound/sound2.wav");
    checkpointSound->doRepeat = false;

    carSettings = GetVehicleSettingsFromFile("data/car.json");
    carSettings.Init();

    CreateCars();
    //carSettings2 = GetVehicleSettingsFromFile("data/car2.json");
    //carSettings2.Init();
    //car->Init(carSettings);
    //car2->Init(carSettings);

    //gPlayers[0].SetVehicle(car);
    //gPlayers[1].SetVehicle(car2);
    //CreateCheckpoint(JPH::Vec3(1, 2, 1));

    mapModel = std::unique_ptr<Model>(LoadModel(mapFilepaths[gMapOption.selectedChoice], MapNodeCallback, LightCallback));
    Phys::LoadMap(*mapModel);
    
    RespawnVehicles();
}


void World::CleanUp()
{
    Vehicle::DestroyAllVehicles();
}


void World::CreateCars()
{
    //car = CreateVehicle();
    //car2 = CreateVehicle();
    // Create a car for each player
    for (int i = 0; i < gNumPlayers; i++) {
        gPlayers[i].CreateAndUseVehicle(carSettings);
    }
        
}


/*
Vehicle& World::GetCar()
{
    //return *car;
    return *gPlayers[0].vehicle;
}


Vehicle& World::GetCar2()
{
    //return *car2;
    return *gPlayers[1].vehicle;
}
*/


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
