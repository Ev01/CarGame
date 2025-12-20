#include "render.h"
#include "render_shadow.h"
#include "render_lights.h"
#include "render_internal.h"
#include "render_ui.h"

#include "convert.h"
#include "camera.h"
#include "model.h"
#include "shader.h"
#include "../glad/glad.h"
#include "world.h"
#include "player.h"
#include "font.h"
#include "main_game.h"
#include "glerr.h"

#include "../vendor/imgui/backends/imgui_impl_sdl3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <SDL3/SDL.h>


static std::vector<Render::Light> lights;
static Render::SunLight sunLight;
static SDL_GLContext context;
static Material grassMat;
static VehicleCamera cam2;
static VehicleCamera cam3;
static unsigned int physFrameCounter = 0;
static const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

/*
 * Render the scene without any shader setup
 */
void Render::RenderSceneRaw(ShaderProg &shader, Player *p)
{
    // Draw map
    DrawMap(shader);

    // Draw all cars
    DrawCars(shader);

    // Draw next checkpoint in race
    if (p != nullptr) {
        DrawCheckpoints(shader, p);
    }

}


void Render::AssimpAddLight(const aiLight *aLight, const aiNode *aNode, aiMatrix4x4 aTransform)
{
    aiQuaternion rotation;
    aiVector3D position;
    glm::mat4 glmTransform = ToGlmMat4(aTransform);
    switch (aLight->mType) {
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
            //newLight.mDirection = toGlmVec3(aLight->mDirection);
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

        /*
        case aiLightSource_SPOT:
            SDL_Log("Adding Spot light %s...", aNode->mName.C_Str());
            SpotLight spotLight;

            aTransform.DecomposeNoScaling(rotation, position);
            glm::vec3 position2;
            position2 = glm::vec3(glmTransform * glm::vec4(ToGlmVec3(aLight->mPosition), 1.0));

            //SDL_Log("light pos (%f, %f, %f)", aLight->mPosition.x, aLight->mPosition.y, aLight->mPosition.z);

            //spotLight.mPosition = ToGlmVec3(position);
            spotLight.mPosition = position2;
            glm::vec3 dir;
            dir = ToGlmVec3(aLight->mDirection);
            spotLight.mDirection = glm::vec3(glmTransform * glm::vec4(dir, 0.0));
            spotLight.mColour = ToGlmVec3(aLight->mColorDiffuse);
            spotLight.mQuadratic = aLight->mAttenuationQuadratic;
            spotLight.mCutoffInner = SDL_cos(aLight->mAngleInnerCone);
            spotLight.mCutoffOuter = SDL_cos(aLight->mAngleOuterCone);

            spotLights.push_back(spotLight);
            break;
        */

        default:
            //SDL_Log("Light not supported");
            break;
    }
            

}


bool Render::Init()
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(
            SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    window = SDL_CreateWindow("Car", 800, 600, flags);

    if (window == NULL) {
        SDL_Log("Could not create window: %s", SDL_GetError());
        return false;
    }

    context = SDL_GL_CreateContext(window);

    // Load opengl functions
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        SDL_Log("Failed to initialise GLAD");
        return false;
    }

    if (!SDL_GL_SetSwapInterval(1)) {
        SDL_Log("Could not turn on VSync");
    }

    uiProj = glm::ortho(0.0f, 800.0f, 0.0f, 600.0f);
    LoadShaders();
    GLERR;

    InitSkybox();
    CreateQuadVAO();

    // Window material (used for checkpoints)
    windowMat.texture = CreateTextureFromFile("data/texture/blending_transparent_window.png");
    //windowMat.texture.SetWrapClamp();
    windowMat.normalMap = gDefaultNormalMap;
    windowMat.roughnessMap = gDefaultTexture;
    windowMat.diffuseColour = glm::vec3(1.0f);

    tachoMeterTex = CreateTextureFromFile("data/texture/tachometer.png");
    tachoNeedleTex = CreateTextureFromFile("data/texture/tachometer_needle.png");

    GLERR;

    // Models
    cubeModel = LoadModel("data/models/cube.gltf");
    quadModel = LoadModel("data/models/quad.gltf");

    GLERR;
    /*
    if (!LoadFont()) {
        return false;
    }
    */
    InitText();

    GLERR;
    // ----- Framebuffer -----
    CreateFBOs();

    GLERR;
    InitShadows();


    // Enable MSAA (anti-aliasing)
    glEnable(GL_MULTISAMPLE);
    // Enable transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLERR;

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    
    SDL_Log("Getting display modes...");
    int numDisplayModes;
    SDL_DisplayMode **displayModes = SDL_GetFullscreenDisplayModes(
            SDL_GetDisplayForWindow(window), &numDisplayModes);
    for (int i = 0; i < numDisplayModes; i++) {
        SDL_Log("Display Mode: w = %d, h = %d @ %.1f Hz",
                displayModes[i]->w, displayModes[i]->h,
                displayModes[i]->refresh_rate);
    }

    return true;
}


float Render::ScreenAspect()
{
    UpdateWindowSize();
    float aspect = (float) screenWidth / (float) screenHeight;
    // Divide aspect by 2.0 for split screen
    if (doSplitScreen && gNumPlayers == 2) {
        //SDL_Log("Dividing");
        aspect /= 2.0;
    }
    //SDL_Log("Aspect: %f, width: %d, height: %d", aspect, screenWidth, screenHeight);
    return aspect;
}


void Render::PhysicsUpdate(double delta)
{
    if (physFrameCounter % 30 == 0) {
        // Sort spot lights from nearest to player to furthest from player.
        // This doesn't need to happen very often.
        
        int numSplitScreens = doSplitScreen ? gNumPlayers : 1;
        SortSpotLights(numSplitScreens);
    }

    physFrameCounter++;
}


void Render::Update(double delta)
{
    DebugGUI();
}


void Render::RenderFrame()
{
    GLERR;

    if (MainGame::gGameState == GAME_IN_WORLD) {
        ShadowPass();
    }

    // Get screen width and height
    UpdateWindowSize();

    GLERR;

    // Set up multisampled framebuffer for rendering
    glBindFramebuffer(GL_FRAMEBUFFER, msFBO);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.7f, 0.6f, 0.2f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLERR;
    
    if (MainGame::gGameState == GAME_IN_WORLD 
            || MainGame::gGameState == GAME_IN_WORLD_PAUSED) {
        RenderSceneSplitScreen();
        //RenderShadowDepthToScreen();
    }


    GLERR;
    
    // Blit multisampled framebuffer onto the regular framebuffer
    glBindFramebuffer(GL_READ_FRAMEBUFFER, msFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
    glBlitFramebuffer(0, 0, screenWidth, screenHeight,
                      0, 0, screenWidth, screenHeight,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    // Render the regular framebuffer to the screen on a quad
    
    
    glDisable(GL_CULL_FACE);
    
    
    glViewport(0, 0, screenWidth, screenHeight);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    
    glUseProgram(screenShader.id);
    glBindVertexArray(quadVAO);
    glBindTexture(GL_TEXTURE_2D, textureColourBuffer);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    
    //RenderShadowDepthToScreen();
    


    // Do UI pass afterwards to avoid postprocessing on UI
    glUseProgram(uiShader.id);
    GuiPass();
    
    GLERR;

    
    SDL_GL_SwapWindow(window);
}


void Render::RenderScene(const Camera &cam, Player *p)
{
    RenderScene(cam.LookAtMatrix(up), cam.projection, true, p);
}


void Render::RenderScene(const glm::mat4 &view, const glm::mat4 &projection, 
                         bool enableSkybox, Player *p)
{
    GLERR;
    glEnable(GL_CULL_FACE);


    glUseProgram(pbrShader.id);

    GLERR;
    pbrShader.SetMat4fv((char*)"projection", glm::value_ptr(projection));
    pbrShader.SetMat4fv((char*)"view", glm::value_ptr(view));

    GLERR;

    // Sun shadow setup
    SetSunShadowUniforms(pbrShader);

    glm::vec3 sunCol = sunLight.mColour / glm::vec3(1.0);
    // Sunlight direction in view space
    glm::vec3 sunDir = glm::vec3(view * glm::vec4(sunLight.mDirection, 0.0));
    pbrShader.SetVec3((char*)"dirLight.direction", glm::value_ptr(sunDir));
    pbrShader.SetVec3((char*)"dirLight.ambient", 0.05, 0.05, 0.05);
    pbrShader.SetVec3((char*)"dirLight.diffuse", glm::value_ptr(sunCol));
    pbrShader.SetVec3((char*)"dirLight.specular", glm::value_ptr(sunCol));

    GLERR;
    // Set uniforms for point lights.
    char uniformName[64];
    int lightNum = 0;
    for (size_t i = 0; i < lights.size(); i++) {
        SDL_snprintf(uniformName, 64, "pointLights[%d].position", lightNum);
        glm::vec3 viewPos = glm::vec3(view * glm::vec4(lights[i].mPosition, 1.0));
        pbrShader.SetVec3(uniformName, glm::value_ptr(viewPos));

        glm::vec3 lightCol = lights[i].mColour / glm::vec3(1.0);
        //glm::vec3 lightCol = glm::vec3(6000.0);
        SDL_snprintf(uniformName, 64, "pointLights[%d].ambient", lightNum);
        pbrShader.SetVec3(uniformName, 0.0, 0.0, 0.0);
        SDL_snprintf(uniformName, 64, "pointLights[%d].diffuse", lightNum);
        pbrShader.SetVec3(uniformName, glm::value_ptr(lightCol));
        SDL_snprintf(uniformName, 64, "pointLights[%d].specular", lightNum);
        pbrShader.SetVec3(uniformName, glm::value_ptr(lightCol));

        //SDL_Log("Colour = (%f, %f, %f)", lights[i].mColour.x, lights[i].mColour.y, lights[i].mColour.z);

        SDL_snprintf(uniformName, 64, "pointLights[%d].quadratic", lightNum);
        pbrShader.SetFloat(uniformName, 1.0f);
        SDL_snprintf(uniformName, 64, "pointLights[%d].constant", lightNum);
        pbrShader.SetFloat(uniformName, 0.0f);
        SDL_snprintf(uniformName, 64, "pointLights[%d].linear", lightNum);
        pbrShader.SetFloat(uniformName, 0.0f);
        lightNum++;
    }

    GLERR;
    
    // Set spot light shadow uniforms
    SetSpotShadowUniforms(pbrShader);
    
    // Set Spot light uniforms
    SetSpotLightUniforms(pbrShader, view);
    GLERR;
    //SDL_Log("Num spot lights: %d", spotLightNum);

    RenderSceneRaw(pbrShader, p);
    GLERR;
    // Draw skybox
    if (enableSkybox) {
        RenderSkybox(view, projection);
    }
    GLERR;
    glDisable(GL_CULL_FACE);
}



void Render::UpdatePlayerCamAspectRatios()
{
    float aspect = Render::ScreenAspect();
    for (int i = 0; i < gNumPlayers; i++) {
        gPlayers[i].cam.cam.SetAspectRatio(aspect);
    }
}

void Render::HandleEvent(SDL_Event *event)
{
    if (event->type == SDL_EVENT_WINDOW_RESIZED) {
        int width = event->window.data1;
        int height = event->window.data2;
        glViewport(0, 0, width, height);

        UpdatePlayerCamAspectRatios();
    }
    else if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_F11) {
        ToggleFullscreen();
    }
}


void Render::DeleteAllLights()
{
    lights.clear();
    //spotLights.clear();
    sunLight.mDirection = glm::vec3(0.0);
    sunLight.mColour = glm::vec3(0.0);
}


void Render::CleanUp()
{
    delete cubeModel;
    delete quadModel;
    glDeleteFramebuffers(1, &fbo);
}


SDL_Window* Render::GetWindow()            { return window; }
SDL_GLContext& Render::GetGLContext()      { return context; }
Render::SunLight& Render::GetSunLight()    { return sunLight; }

