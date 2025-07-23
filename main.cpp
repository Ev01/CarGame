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


double delta, lastFrame;
// Array of FPS values of the last few frames. Current frame is at
// fpsRecordsPosition. Used to calculate average FPS.
double fpsRecords[FPS_RECORD_SIZE];
int fpsRecordPosition = 0;
double averageFps = 0.0;
float physicsTime = 0;


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


static double GetSeconds()
{
    return (double) SDL_GetTicksNS() / (double) SDL_NS_PER_SECOND;
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

    // Init physics system
    Phys::SetupJolt();
    Phys::SetupSimulation();

    World::Init();

    glViewport(0, 0, 800, 600);
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

    // Debug FPS window
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
    static bool dontSkipPhysicsStep = false;
    ImGui::Checkbox("Dont skip physics step", &dontSkipPhysicsStep);
    static double fpsLimit = 300.0;
    const double fpsSliderMin = 0.0;
    const double fpsSliderMax = 300.0;
    ImGui::SliderScalar("FPS Limit", ImGuiDataType_Double, &fpsLimit, 
                        &fpsSliderMin, &fpsSliderMax);
    ImGui::Text("Current FPS: %f", 1.0 / delta);
    ImGui::Text("Average %d frames: %f", FPS_RECORD_SIZE, averageFps);
    ImGui::End();
        
    // Do physics step
    physicsTime += delta;
    if (dontSkipPhysicsStep) {
        World::PrePhysicsUpdate(PHYSICS_STEP_TIME);
        World::ProcessInput();
        Phys::ProcessInput();

        Phys::PhysicsStep(PHYSICS_STEP_TIME);
        Render::PhysicsUpdate(PHYSICS_STEP_TIME);
        physicsTime = 0.0;
    }
    else {
        while (physicsTime >= PHYSICS_STEP_TIME) {
            World::PrePhysicsUpdate(PHYSICS_STEP_TIME);
            World::ProcessInput();
            Phys::ProcessInput();

            Phys::PhysicsStep(PHYSICS_STEP_TIME);
            Render::PhysicsUpdate(PHYSICS_STEP_TIME);
            physicsTime -= PHYSICS_STEP_TIME;
        }
    }

    // Update
    Audio::Update();
    Render::Update(delta);
    World::Update(delta);

    // Render
    Render::RenderFrame();

    // Limit FPS
    if (fpsLimit > 0.1) {
        double delta2 = GetSeconds() - lastFrame;

        double sfp = 1.0 / fpsLimit;
        if (delta2 < sfp) {
            double toDelaySeconds = (sfp - delta2);
            SDL_DelayPrecise((Uint64)(toDelaySeconds * 1000000000.0));
        }
    }
    return SDL_APP_CONTINUE;
}


void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    World::CleanUp();
    Phys::CleanUp();
    Render::CleanUp();
}
    

