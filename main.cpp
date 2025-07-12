#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "vendor/imgui/imgui.h"
#include "vendor/imgui/backends/imgui_impl_opengl3.h"
#include "vendor/imgui/backends/imgui_impl_sdl3.h"

#include <string>
#include <memory>

#include "physics.h"
#include "vehicle.h"
#include "convert.h"
#include "world.h"

#include "glad/glad.h"
#include "model.h"
#include "camera.h"
#include "input.h"
#include "audio.h"
#include "render.h"

#define MOUSE_SENSITIVITY 0.001f
#define CAM_SPEED 4.0f
#define PHYSICS_STEP_TIME 1.0 / 60
#define FPS_RECORD_SIZE 100


std::unique_ptr<Model> monkeyModel;
std::unique_ptr<Model> cylinderModel;
std::unique_ptr<Model> carModel;
std::unique_ptr<Model> wheelModel;
std::unique_ptr<Model> mapModel;
//std::unique_ptr<Model> mapModel2;
std::unique_ptr<Model> map1Model;
std::unique_ptr<Model> *currentMap;

static glm::vec3 mapSpawnPoint = glm::vec3(0.0f);

const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

double delta, lastFrame;
// Array of FPS values of the last few frames. Current frame is at
// fpsRecordsPosition. Used to calculate average FPS.
double fpsRecords[FPS_RECORD_SIZE];
int fpsRecordPosition = 0;
double averageFps = 0.0;
float physicsTime = 0;

float yaw = -SDL_PI_F / 2.0;
float pitch;

VehicleSettings carSettings;

/*
 * Records the newFps into the fpsRecords array and updates the average FPS.
 */
static void RecordFps(double newFps)
{
    fpsRecordPosition++;
    if (fpsRecordPosition >= FPS_RECORD_SIZE) {
        fpsRecordPosition -= FPS_RECORD_SIZE; 
    }
    averageFps -= fpsRecords[fpsRecordPosition] / (double) FPS_RECORD_SIZE;
    fpsRecords[fpsRecordPosition] = newFps;
    averageFps += newFps / (double) FPS_RECORD_SIZE;
}



double GetSeconds()
{
    return (double) SDL_GetTicksNS() / (double) SDL_NS_PER_SECOND;
}

bool CarNodeCallback(const aiNode *node, aiMatrix4x4 transform)
{
    aiQuaternion aRotation;
    aiVector3D aPosition;
    aiVector3D aScale;
    transform.Decompose(aScale, aRotation, aPosition);

    JPH::RMat44 joltTransform = ToJoltMat4(transform);
    JPH::Vec3 position = joltTransform.GetTranslation();
    if (SDL_strcmp(node->mName.C_Str(), "WheelPosFR") == 0) {
        //SDL_Log("FR, %s", node->mName.C_Str());

        carSettings.AddWheel(position, true);
    }
    else if (SDL_strcmp(node->mName.C_Str(), "WheelPosFL") == 0) {
        //SDL_Log("FL");
        carSettings.AddWheel(position, true);
    }
    else if (SDL_strcmp(node->mName.C_Str(), "WheelPosRR") == 0) {
        //SDL_Log("RR");
        carSettings.AddWheel(position, false);
    }
    else if (SDL_strcmp(node->mName.C_Str(), "WheelPosRL") == 0) {
        carSettings.AddWheel(position, false);
    }
    else if (SDL_strncmp(node->mName.C_Str(), "CollisionBox", 12) == 0) {
        carSettings.AddCollisionBox(ToJoltVec3(aPosition), ToJoltVec3(aScale));
    }
    else if (SDL_strcmp(node->mName.C_Str(), "HeadLightLeft") == 0) {
        carSettings.headLightLeftTransform = joltTransform;
    }
    else if (SDL_strcmp(node->mName.C_Str(), "HeadLightRight") == 0) {
        carSettings.headLightRightTransform = joltTransform;
    }
    return true;
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
    return true;
}


static void LightCallback(const aiLight *aLight, const aiNode *aNode,
                           aiMatrix4x4 aTransform)
{
    World::AssimpAddLight(aLight, aNode, aTransform);
    Render::AssimpAddLight(aLight, aNode, aTransform);
}



/*
static void GLAPIENTRY GLErrorCallback(GLenum source, GLenum type,
                                       GLuint, GLenum severity,
                                       GLsizei legnth, const GLchar* message,
                                       const void* userParam)
{
    SDL_Log("GL CALLBACK: type = 0x%x, severity = 0x%x, message = %s",
            //(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR**" : ""),
            type, severity, message);
}
*/


static void AssimpTest()
{
    Assimp::Importer importer;
    /*const aiScene *scene = */importer.ReadFile(
            "models/no_tex_map.gltf",
            aiProcess_Triangulate 
            | aiProcess_FlipUVs 
            | aiProcess_CalcTangentSpace);
}




SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO);


    if (!Render::Init()) {
        return SDL_APP_FAILURE;
    }

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(Render::GetWindow(), Render::GetGLContext());
    ImGui_ImplOpenGL3_Init(nullptr);

    InitDefaultTexture();

    // Audio
    Audio::Init();

    Phys::SetupJolt();
    Phys::CreateCars();

    carSettings = GetVehicleSettingsFromFile("data/car.json");

    // Load model
    //monkeyModel = LoadModel("models/monkey.obj");
    cylinderModel = std::unique_ptr<Model>(LoadModel("models/cylinder.obj"));
    carModel = std::unique_ptr<Model>(LoadModel("models/silvia_s13_ver2.gltf", CarNodeCallback));
    wheelModel = std::unique_ptr<Model>(LoadModel("models/silvia_wheel.gltf"));
    mapModel = std::unique_ptr<Model>(LoadModel("models/no_tex_map.gltf", MapNodeCallback, LightCallback));
    //map1Model = LoadModel("models/map1.gltf", NULL, Render::AssimpAddLight);

    //currentMap = &mapModel;

    Phys::SetupSimulation();
    Phys::GetCar().Init(carSettings);
    Phys::GetCar2().Init(carSettings);
    Phys::GetBodyInterface().SetPosition(Phys::GetCar2().mBody->GetID(), JPH::Vec3(6.0, 0, 0), JPH::EActivation::Activate);
    
    Phys::LoadMap(*mapModel);

    glViewport(0, 0, 800, 600);

    //SDL_SetWindowRelativeMouseMode(Render::GetWindow(), true);
    /*
    if (!SDL_SetWindowSurfaceVSync(window, 1)) {
        SDL_Log("Couldn't set VSync: %s", SDL_GetError());
    }
    */
    glEnable(GL_DEPTH_TEST);

    lastFrame = GetSeconds();

    return SDL_APP_CONTINUE;
}


SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    ImGuiIO &io = ImGui::GetIO(); (void)io;
    ImGui_ImplSDL3_ProcessEvent(event);
    // Do not propogate events to engine when captured by ImGui.
    if ((event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP)
            && io.WantCaptureKeyboard) {
        return SDL_APP_CONTINUE;
    }
    else if ((event->type == SDL_EVENT_MOUSE_BUTTON_DOWN
            || event->type == SDL_EVENT_MOUSE_BUTTON_UP
            || event->type == SDL_EVENT_MOUSE_MOTION)
            && io.WantCaptureMouse) {
        return SDL_APP_CONTINUE;
    }

    Input::HandleEvent(event);
    Render::HandleEvent(event);

    if (event->type == SDL_EVENT_MOUSE_MOTION) {
        yaw += event->motion.xrel * MOUSE_SENSITIVITY;
        pitch -= event->motion.yrel * MOUSE_SENSITIVITY;
        pitch = SDL_clamp(pitch, -SDL_PI_F / 2.0 + 0.1, SDL_PI_F / 2.0 - 0.1);
    }
    else if (event->type == SDL_EVENT_KEY_DOWN && event->key.scancode == SDL_SCANCODE_F) {
        SDL_Log("FPS: %f", 1.0/delta);
    }
    /*
    else if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_TAB) {
        SDL_SetWindowRelativeMouseMode(Render::GetWindow(), false);
    }
    else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == 1) {
        SDL_SetWindowRelativeMouseMode(Render::GetWindow(), true);
    }
    */


    return SDL_APP_CONTINUE;
}


SDL_AppResult SDL_AppIterate(void *appstate)
{
    delta = GetSeconds() - lastFrame;
    lastFrame = GetSeconds();

    RecordFps(1.0 / delta);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Maps");
    const char* items[] = {"Map01", "Map02", "simple_map"};
    static int currentItem = 0;
    ImGui::Combo("Map", &currentItem, items, 3);
    if (ImGui::Button("Change map")) {
        //Render::DeleteAllLights();
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
        bodyInterface.SetPosition(Phys::GetCar().mBody->GetID(), ToJoltVec3(mapSpawnPoint), JPH::EActivation::Activate);
        bodyInterface.SetPosition(Phys::GetCar2().mBody->GetID(), ToJoltVec3(mapSpawnPoint) + JPH::Vec3(6.0, 0, 0), JPH::EActivation::Activate);
    }
    if (ImGui::Button("Assimp Test")) {
        AssimpTest();
    }
    if (ImGui::Button("Map load test")) {
        Model *testMap = LoadModel("models/no_tex_map.gltf");
        delete testMap;
    }
    ImGui::End();

    ImGui::Begin("FPS");
    int interval;
    if (SDL_GL_GetSwapInterval(&interval)) {
        bool vSync = interval != 0;
        ImGui::Checkbox("VSync", &vSync);
        interval = (int) vSync;
        SDL_GL_SetSwapInterval(interval);
    }
    else {
        ImGui::Text("Couldn't get GL swap interval (VSync)");
    }
    static double fpsLimit = 300.0;
    const double fpsSliderMin = 0.0;
    const double fpsSliderMax = 300.0;
    //ImGui::SliderFloat("FPS Limit", &fpsLimit, 0.0, 300.0);
    ImGui::SliderScalar("FPS Limit", ImGuiDataType_Double, &fpsLimit, 
                        &fpsSliderMin, &fpsSliderMax);
    ImGui::Text("Current FPS: %f", 1.0 / delta);
    ImGui::Text("Average %d frames: %f", FPS_RECORD_SIZE, averageFps);
    ImGui::End();
        

    Camera &cam = Render::GetCamera();

    // Do physics step
    physicsTime += delta;
    while (physicsTime >= PHYSICS_STEP_TIME) {
        Phys::SetForwardDir(ToJoltVec3(cam.dir));
        Phys::ProcessInput();
        Phys::PhysicsStep(PHYSICS_STEP_TIME);
        Render::PhysicsUpdate(PHYSICS_STEP_TIME);
        physicsTime -= PHYSICS_STEP_TIME;
    }

    // Audio
    Audio::Update();
    Render::Update(delta);

    Render::RenderFrame(*mapModel, *carModel, *wheelModel);

    if (fpsLimit > 0.0) {
        double delta2 = GetSeconds() - lastFrame;

        double sfp = 1.0 / fpsLimit;
        //SDL_Log("delta: %f", delta);
        if (delta2 < sfp) {
            double toDelaySeconds = (sfp - delta2);
            //SDL_Log("Delta is %f. Delaying %f nanoseconds. SFP: %f", delta2, toDelaySeconds, sfp);
            //double delayBefore = GetSeconds();
            SDL_DelayPrecise((Uint64)(toDelaySeconds * 1000000000.0));
            //SDL_Log("Real delay: %f", GetSeconds() - delayBefore);
        }
    }
    return SDL_APP_CONTINUE;
}


void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    Phys::CleanUp();
    Render::CleanUp();
}
    

