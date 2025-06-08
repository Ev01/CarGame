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

#include "physics.h"
#include "vehicle.h"
#include "convert.h"

#include "glad/glad.h"
#include "model.h"
#include "camera.h"
#include "input.h"
#include "audio.h"
#include "render.h"

#define MOUSE_SENSITIVITY 0.001f
#define CAM_SPEED 4.0f
#define PHYSICS_STEP_TIME 1.0 / 60



Model monkeyModel;
Model cylinderModel;
Model carModel;
Model wheelModel;
Model mapModel;
Camera cam;

const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

double delta, lastFrame;
float physicsTime = 0;

float yaw = -SDL_PI_F / 2.0;
float pitch;


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

        Phys::GetCar().AddWheel(position, true);
    }
    else if (SDL_strcmp(node->mName.C_Str(), "WheelPosFL") == 0) {
        //SDL_Log("FL");
        Phys::GetCar().AddWheel(position, true);
    }
    else if (SDL_strcmp(node->mName.C_Str(), "WheelPosRR") == 0) {
        //SDL_Log("RR");
        Phys::GetCar().AddWheel(position, false);
    }
    else if (SDL_strcmp(node->mName.C_Str(), "WheelPosRL") == 0) {
        Phys::GetCar().AddWheel(position, false);
    }
    else if (SDL_strncmp(node->mName.C_Str(), "CollisionBox", 12) == 0) {
        Phys::GetCar().AddCollisionBox(ToJoltVec3(aPosition), ToJoltVec3(aScale));
    }
    return true;
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


    // Load model
    monkeyModel = LoadModel("models/monkey.obj");
    cylinderModel = LoadModel("models/cylinder.obj");
    carModel = LoadModel("models/mycar.gltf", CarNodeCallback);
    wheelModel = LoadModel("models/wheel.gltf");
    mapModel = LoadModel("models/simple_map.gltf", NULL, Render::AssimpAddLight);
    //mapModel = LoadModel("models/map1.gltf", NULL, Render::AssimpAddLight);

    cam.pos.z = 6.0f;
    cam.SetYawPitch(yaw, pitch);

    Phys::SetupSimulation();
    Phys::LoadMap(mapModel);

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


    // Do physics step
    physicsTime += delta;
    while (physicsTime >= PHYSICS_STEP_TIME) {
        Phys::SetForwardDir(ToJoltVec3(cam.dir));
        Phys::ProcessInput();
        Phys::PhysicsStep(PHYSICS_STEP_TIME);
        physicsTime -= PHYSICS_STEP_TIME;
    }

    // Audio
    Audio::Update();

    JPH::Vec3 carPosJolt = Phys::GetCar().GetPos();
    glm::vec3 carPos = ToGlmVec3(carPosJolt);
    JPH::Quat carRot = Phys::GetCar().GetRotation();
    JPH::Vec3 carDir = carRot.RotateAxisX();
    float carYaw = SDL_PI_F - SDL_atan2f(carDir.GetX(), carDir.GetZ());
    float yawOffset;
    if (Input::GetGamepad()) {
        yawOffset = SDL_PI_F / 2.0f * Input::GetGamepadAxis(SDL_GAMEPAD_AXIS_RIGHTX);
    }
    //SDL_Log("Car yaw: %f, x: %f, z: %f", carYaw, carDir.GetX(), carDir.GetZ());
    cam.SetFollowSmooth(carYaw + yawOffset, -0.2f, 14.0f, carPos, 3.0f * delta, 10.0f * delta);
    // Cast ray for camera
    // Doing it this way still makes the camera clip through a little bit. To
    // fix this, a spherecast could be used with a radius equal to the camera's
    // near-plane clipping distance. This code should also be moved to the
    // camera and applied before smoothing. For this, the camera's target
    // position may need to be stored to separate this code from smoothing.
    JPH::Vec3 newCamPos;
    JPH::Vec3 camPosJolt = ToJoltVec3(cam.pos);
    JPH::Vec3 camToCar = carPosJolt - camPosJolt;
    JPH::IgnoreSingleBodyFilter carBodyFilter = JPH::IgnoreSingleBodyFilter(Phys::GetCar().mBody->GetID());
    bool hadHit = Phys::CastRay(carPosJolt - camToCar/2.0,  -camToCar/2.0,  newCamPos, carBodyFilter);
    if (hadHit) {
        cam.pos = ToGlmVec3(newCamPos);
    }


    //cam.SetOrbit(14.0f, 4.0f, carPos);
    // Move Camera
    /*
    if (Input::IsScanDown(SDL_SCANCODE_A)) {
        cam.pos -= cam.Right(up) * CAM_SPEED * (float)delta;
    }
    if (Input::IsScanDown(SDL_SCANCODE_D)) {
        cam.pos += cam.Right(up) * CAM_SPEED * (float)delta;
    }
    if (Input::IsScanDown(SDL_SCANCODE_W)) {
        cam.pos += cam.dir * CAM_SPEED * (float)delta;
    }
    if (Input::IsScanDown(SDL_SCANCODE_S)) {
        cam.pos -= cam.dir * CAM_SPEED * (float)delta;
    }
    */
    
    Render::RenderFrame(cam, mapModel, carModel, wheelModel);


    double delta2 = GetSeconds() - lastFrame;

    constexpr double sfp = 1.0 / 300.0;
    //SDL_Log("delta: %f", delta);
    if (delta2 < sfp) {
        double toDelaySeconds = (sfp - delta2);
        //SDL_Log("Delta is %f. Delaying %f nanoseconds. SFP: %f", delta2, toDelaySeconds, sfp);
        //double delayBefore = GetSeconds();
        SDL_DelayPrecise((Uint64)(toDelaySeconds * 1000000000.0));
        //SDL_Log("Real delay: %f", GetSeconds() - delayBefore);
    }
    return SDL_APP_CONTINUE;
}


void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    Phys::CleanUp();
    Render::CleanUp();
}
    

