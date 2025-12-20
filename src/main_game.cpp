#include "main_game.h"

#include <SDL3/SDL.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui/backends/imgui_impl_opengl3.h"
#include "../vendor/imgui/backends/imgui_impl_sdl3.h"


#include "physics.h"
#include "world.h"
#include "texture.h"

#include "../glad/glad.h"
#include "input.h"
#include "input_mapping.h"
#include "audio.h"
#include "render.h"
#include "player.h"
#include "font.h"
#include "ui.h"
#include "vehicle.h"

#include <ft2build.h>
#include FT_FREETYPE_H



#define MOUSE_SENSITIVITY 0.001f
#define CAM_SPEED 4.0f
#define PHYSICS_STEP_TIME 1.0 / 60
#define FPS_RECORD_SIZE 100


static double delta, lastFrame;
// Array of FPS values of the last few frames. Current frame is at
// fpsRecordsPosition. Used to calculate average FPS.
static double fpsRecords[FPS_RECORD_SIZE];
static int fpsRecordPosition = 0;
static double averageFps = 0.0;
static float physicsTime = 0;

static bool dontSkipPhysicsStep = false;
static double fpsLimit = 300.0;

static bool requestToQuit = false;

GameState MainGame::gGameState = GAME_PRESS_START_SCREEN;

/*
 * Records the newFps into the fpsRecords array and updates the average FPS.
 */
static void RecordFps(double newFps)
{
    fpsRecordPosition++;
    if (fpsRecordPosition >= FPS_RECORD_SIZE) {
        fpsRecordPosition -= FPS_RECORD_SIZE; 
    }
    averageFps -= fpsRecords[fpsRecordPosition] / FPS_RECORD_SIZE;
    fpsRecords[fpsRecordPosition] = newFps;
    averageFps += newFps / FPS_RECORD_SIZE;
}


// Return seconds since application start (accurate)
static double GetSeconds()
{
    return (double) SDL_GetTicksNS() / SDL_NS_PER_SECOND;
}


// Step the physics simulation a single time
static void DoPhysicsStep()
{
    World::PrePhysicsUpdate(PHYSICS_STEP_TIME);

    Phys::PhysicsStep(PHYSICS_STEP_TIME);

    Render::PhysicsUpdate(PHYSICS_STEP_TIME);
    Player::PhysicsUpdateAllPlayers(PHYSICS_STEP_TIME);
}


// Decide how many physics steps to do this frame and do them
static void PhysicsUpdate()
{
    physicsTime += delta;
    if (dontSkipPhysicsStep) {
        DoPhysicsStep();
        physicsTime = 0.0;
    }
    else {
        while (physicsTime >= PHYSICS_STEP_TIME) {
            DoPhysicsStep();
            physicsTime -= PHYSICS_STEP_TIME;
        }
    }
}


// Called before any other update function in the frame. Best used
// to update input related things.
static void InputUpdate()
{
    World::InputUpdate();
    Phys::InputUpdate();
    Player::InputUpdateAllPlayers();
}


// Called just before rendering. 
static void FrameUpdate()
{
    Input::Update();
    Audio::Update();
    Render::Update(delta);
    World::Update(delta);

    gPlayers[0].DebugGUI();
}


static void LimitFPS()
{
    if (fpsLimit > 0.1) { // Disable FPS limit if at 0
        // Time since the start of the frame
        double delta2 = GetSeconds() - lastFrame;
        // Minimum seconds the frame should last
        double spf = 1.0 / fpsLimit;
        if (delta2 < spf) {
            double toDelaySeconds = (spf - delta2);
            SDL_Log("to delay: %f", toDelaySeconds);
            SDL_DelayPrecise((Uint64)(toDelaySeconds * SDL_NS_PER_SECOND));
        }
    }
}


static void DebugGUI()
{
    // Debug FPS window
    ImGui::Begin("FPS", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);
    int interval;
    if (SDL_GL_GetSwapInterval(&interval)) {
        bool vSync = interval != 0;
        ImGui::Checkbox("VSync", &vSync);
        interval = (int) vSync;
        SDL_GL_SetSwapInterval(interval);
        //SDL_Log("VSync: %d", interval);
    }
    else {
        ImGui::Text("Couldn't get GL swap interval (VSync)");
    }
    
    ImGui::Checkbox("Dont skip physics step", &dontSkipPhysicsStep);
    const double fpsSliderMin = 0.0;
    const double fpsSliderMax = 300.0;
    ImGui::SliderScalar("FPS Limit", ImGuiDataType_Double, &fpsLimit, 
                        &fpsSliderMin, &fpsSliderMax);
    ImGui::Text("Current FPS: %f", 1.0 / delta);
    ImGui::Text("Average %d frames: %f", FPS_RECORD_SIZE, averageFps);
    ImGui::End();

}


static void Pause()
{
    if (MainGame::gGameState == GAME_IN_WORLD) {
        MainGame::gGameState = GAME_IN_WORLD_PAUSED;
    }
}


static void Unpause()
{
    if (MainGame::gGameState == GAME_IN_WORLD_PAUSED) {
        MainGame::gGameState = GAME_IN_WORLD;
    }
}


void MainGame::StartWorld()
{
    if (gGameState == GAME_IN_WORLD) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Tried to start world, but already started");
        return;
    }

    UI::CloseAllMenus();
    Phys::SetupSimulation();
    World::Init();
    // Start race immediatley after starting the world.
    World::BeginRaceCountdown();
    Render::UpdatePlayerCamAspectRatios();
    MainGame::gGameState = GAME_IN_WORLD;
}


void MainGame::EndWorld()
{
    if (gGameState == GAME_PRESS_START_SCREEN) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Tried to end world, but world hasn't been started");
        return;
    }

    World::CleanUp();
    MainGame::gGameState = GAME_PRESS_START_SCREEN;
    UI::CloseAllMenus();
    UI::OpenMenu(&UI::mainMenu);
}


void MainGame::Quit()
{
    SDL_Log("Quitting game...");
    requestToQuit = true;
}


SDL_AppResult MainGame::Init()
{
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, "Car Game");
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, "game");
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO);

    if (!Render::Init()) {
        return SDL_APP_FAILURE;
    }
    if (!Font::Init()) {
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

    // Input
    // Allow multiple keyboards
    const char temp = 1;
    SDL_SetHint(SDL_HINT_WINDOWS_RAW_KEYBOARD, &temp);
    Input::Init();

    // Init physics system
    Phys::SetupJolt();

    // Players
    //Player::AddPlayer();

    UI::OpenMenu(&UI::mainMenu);


    glViewport(0, 0, 800, 600);
    glEnable(GL_DEPTH_TEST);

    lastFrame = GetSeconds();

    
    int numKeyboards = 0;
    SDL_KeyboardID *ids = SDL_GetKeyboards(&numKeyboards);
    SDL_Log("Keyboards:");
    for (int i = 0; i < numKeyboards; i++) {
        SDL_Log("%s with id %d", SDL_GetKeyboardNameForID(ids[i]), ids[i]);
    }
    

    return SDL_APP_CONTINUE;
}


SDL_AppResult MainGame::HandleEvent(SDL_Event *event) 
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

    /*
    if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_RETURN
    //if (Input::GetNumRealKeyboards() > 0
            && gGameState == GAME_PRESS_START_SCREEN) {
        StartWorld();
    }
    */

    if (gGameState == GAME_IN_WORLD) {
        //if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE) {
        if (gDefaultControlScheme.EventMatchesActionJustPressed(event, ACTION_PAUSE)
                && World::GetRaceState() != RACE_ENDED) {
            UI::OpenMenu(UI::GetPauseMenu());
            Pause();
        }
    }


    //if (gGameState == GAME_PRESS_START_SCREEN) {
    Input::HandleEvent(event);
    UI::HandleEvent(event);
    Render::HandleEvent(event);

    return SDL_APP_CONTINUE;
}


SDL_AppResult MainGame::Update()
{
    if (requestToQuit) {
        return SDL_APP_SUCCESS;
    }

    delta = GetSeconds() - lastFrame;
    lastFrame = GetSeconds();

    RecordFps(1.0 / delta);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    InputUpdate();
    UI::Update();
    DebugGUI();

    switch (gGameState) {
        case GAME_PRESS_START_SCREEN:
            FrameUpdate(); 
            break;
        case GAME_IN_WORLD_PAUSED:
            // Unpause if the user goes out of the pause menu
            if (UI::GetCurrentMenu() == nullptr) Unpause();
            break;
        case GAME_IN_WORLD:
            PhysicsUpdate();
            FrameUpdate(); 
            break;
    }

    //FrameUpdate(); 
    Render::RenderFrame();

    // A bit of a hack to make sure no ImGui window takes focus when the
    // program starts up.
    static bool isFirstFrame = true;
    if (isFirstFrame) {
        ImGui::SetWindowFocus(nullptr);
        isFirstFrame = false;
    }


    LimitFPS();
    Input::NewFrame();

    return SDL_APP_CONTINUE;
}


void MainGame::CleanUp()
{
    World::CleanUp();
    Phys::CleanUp();
    Render::CleanUp();
}
