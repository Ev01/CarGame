#include "world.h"
#include "render.h"
#include "convert.h"
#include "vehicle.h"
#include "model.h"
#include "physics.h"

#include "vendor/imgui/imgui.h"

#include <assimp/matrix4x4.h>
#include <assimp/types.h>
#include <assimp/vector3.h>

#include <glm/glm.hpp>

#include <vector>

static std::vector<Render::SpotLight*> spotLights;
static Vehicle *car;
static Vehicle *car2;
static VehicleSettings carSettings;
static VehicleSettings carSettings2;

static std::unique_ptr<Model> mapModel;
static glm::vec3 mapSpawnPoint = glm::vec3(0.0f);
//static Model *currentMap;


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
    mapModel.reset(LoadModel(modelFileName, MapNodeCallback, LightCallback));
    Phys::LoadMap(*mapModel);
}




void World::PrePhysicsUpdate(float delta)
{
    car->Update(delta);
    car2->Update(delta);
}


void World::Update(float delta)
{
    ImGui::Begin("Maps");
    const char* items[] = {"Map01", "Map02", "simple_map"};
    static int currentItem = 0;
    ImGui::Combo("Map", &currentItem, items, 3);
    if (ImGui::Button("Change map")) {
        World::DestroyAllLights();
        Render::DeleteAllLights();
        mapSpawnPoint = glm::vec3(0.0f);
        JPH::BodyInterface &bodyInterface = Phys::GetBodyInterface();
        Phys::UnloadMap();
        switch(currentItem) {
            case 0:
                SDL_Log("sizeof: %lld", sizeof(*mapModel));
                mapModel.reset(LoadModel("models/no_tex_map.gltf",
                               MapNodeCallback, LightCallback));
                Phys::LoadMap(*mapModel);
                break;
            case 1:
                mapModel.reset(LoadModel("models/map1.gltf",
                               MapNodeCallback, LightCallback));
                Phys::LoadMap(*mapModel);
                break;
            case 2:
                mapModel.reset(LoadModel("models/simple_map.gltf",
                               MapNodeCallback, LightCallback));
                Phys::LoadMap(*mapModel);
                break;

        }
        bodyInterface.SetPosition(World::GetCar().mBody->GetID(), ToJoltVec3(mapSpawnPoint), JPH::EActivation::Activate);
        bodyInterface.SetPosition(World::GetCar2().mBody->GetID(), ToJoltVec3(mapSpawnPoint) + JPH::Vec3(6.0, 0, 0), JPH::EActivation::Activate);
    }
    ImGui::End();
}


void World::ProcessInput()
{
    car->ProcessInput(false);
    car2->ProcessInput(true);
}

void World::Init()
{
    CreateCars();
    carSettings = GetVehicleSettingsFromFile("data/car.json");
    carSettings2 = GetVehicleSettingsFromFile("data/car2.json");
    carSettings.Init();
    carSettings2.Init();
    car->Init(carSettings);
    car2->Init(carSettings2);

    mapModel = std::unique_ptr<Model>(LoadModel("models/no_tex_map.gltf", MapNodeCallback, LightCallback));
    Phys::LoadMap(*mapModel);
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
