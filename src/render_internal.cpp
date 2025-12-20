#include "render_internal.h"
#include "render.h"
#include "render_shadow.h"

#include "../glad/glad.h"
#include "glerr.h"
#include "convert.h"
#include "model.h"
#include "world.h"
#include "vehicle.h"
#include "player.h"
#include "shader.h"
#include "ui.h"

#include "../vendor/imgui/imgui.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

unsigned int textVAO, textVBO;
unsigned int quadVAO;
unsigned int quadVBO;
unsigned int uiQuadVAO;
unsigned int uiQuadVBO;
unsigned int fbo;
unsigned int rbo;
unsigned int textureColourBuffer;
unsigned int msFBO;
unsigned int msRBO;
unsigned int msTexColourBuffer;
int screenWidth;
int screenHeight;
Model *cubeModel;
Model *quadModel;
Material windowMat;
Texture tachoMeterTex;
Texture tachoNeedleTex;
glm::mat4 uiProj;

ShaderProg simpleDepthShader;
ShaderProg pbrShader;
ShaderProg screenShader;
ShaderProg uiShader;
ShaderProg textShader;

bool doSplitScreen = true;
SDL_Window *window;

static ShaderProg skyboxShader;
static ShaderProg rawScreenShader;
static ShaderProg samplerArrayTestShader;

static Texture skyboxTex;
static Texture grassTex;
static unsigned int skyboxVAO;
static unsigned int skyboxVBO;


static float skyboxVertices[] = {
    // positions          
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};

static float quadVertices[] = {
    // Positions  // Tex Coords
    -1.0f,  1.0f,  0.0f, 1.0f,    // Top left
     1.0f,  1.0f,  1.0f, 1.0f,    // Top right
     1.0f, -1.0f,  1.0f, 0.0f,    // Bottom right
     1.0f, -1.0f,  1.0f, 0.0f,    // Bottom right
    -1.0f, -1.0f,  0.0f, 0.0f,    // Bottom left
    -1.0f,  1.0f,  0.0f, 1.0f     // Top left
};

static float uiQuadVertices[] = {
    // Positions  // Tex Coords
    0.0f, 1.0f,   0.0f, 1.0f,    // Top left
    1.0f, 1.0f,   1.0f, 1.0f,    // Top right
    1.0f, 0.0f,   1.0f, 0.0f,    // Bottom right
    1.0f, 0.0f,   1.0f, 0.0f,    // Bottom right
    0.0f, 0.0f,   0.0f, 0.0f,    // Bottom left
    0.0f, 1.0f,   0.0f, 1.0f     // Top left
};

void Render::CreateFramebuffer(unsigned int *aFBO, unsigned int *aCbTex, unsigned int *aRBO, 
                              unsigned int aWidth, unsigned int aHeight)
{
    glGenFramebuffers(1, aFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, *aFBO);

    // Create texture for frame buffer
    glGenTextures(1, aCbTex);
    glBindTexture(GL_TEXTURE_2D, *aCbTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, aWidth, aHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    // Attach texture to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *aCbTex, 0);
    // Create renderbuffer for depth and stencil components
    glGenRenderbuffers(1, aRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, *aRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, aWidth, aHeight);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    // Attach render buffer to framebuffer
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, *aRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        SDL_Log("Error: Framebuffer is not complete!");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void Render::CreateMSFramebuffer(unsigned int *aFBO, unsigned int *aCbTex, unsigned int *aRBO,
                                unsigned int aWidth, unsigned int aHeight)
{
    glGenFramebuffers(1, aFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, *aFBO);
    GLERR;

    // Create texture for frame buffer
    glGenTextures(1, aCbTex);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, *aCbTex);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB32F, aWidth, aHeight, GL_TRUE);
    GLERR;
    //glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    GLERR;
    // Attach texture to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, *aCbTex, 0);
    GLERR;
    // Create renderbuffer for depth and stencil components
    glGenRenderbuffers(1, aRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, *aRBO);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, aWidth, aHeight);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    GLERR;
    // Attach render buffer to framebuffer
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, *aRBO);
    GLERR;

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        SDL_Log("Error: Framebuffer is not complete!");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void Render::CreateFBOs()
{
    // if any objects are already existant, delete them before creating them
    // again.
    if (fbo != 0) glDeleteFramebuffers(1, &fbo);
    if (textureColourBuffer != 0) glDeleteTextures(1, &textureColourBuffer);
    if (rbo != 0) glDeleteRenderbuffers(1, &rbo);
    CreateFramebuffer(&fbo, &textureColourBuffer, &rbo, screenWidth, screenHeight);
    GLERR;
    if (msFBO != 0) glDeleteFramebuffers(1, &msFBO);
    if (msTexColourBuffer != 0) glDeleteTextures(1, &msTexColourBuffer);
    if (msRBO != 0) glDeleteRenderbuffers(1, &msRBO);
    CreateMSFramebuffer(&msFBO, &msTexColourBuffer, &msRBO, screenWidth, screenHeight);
    GLERR;
}


void Render::RenderSkybox(glm::mat4 view, glm::mat4 projection)
{
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    // Remove translation component from view matrix
    glm::mat4 skyboxView = glm::mat4(glm::mat3(view));
    glUseProgram(skyboxShader.id);

    GLERR;
    skyboxShader.SetMat4fv((char*)"projection", glm::value_ptr(projection));
    skyboxShader.SetMat4fv((char*)"view", glm::value_ptr(skyboxView));
    skyboxShader.SetInt((char*)"skybox", 0);

    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex.id);
    glBindVertexArray(skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
}


void Render::DrawMap(ShaderProg &shader)
{
    GLERR;
    Model &mapModel = World::GetCurrentMapModel();
    glm::mat4 model = glm::mat4(1.0f);
    GLERR;
    shader.SetMat4fv((char*)"model", glm::value_ptr(model));
    GLERR;
    mapModel.Draw(shader);
    GLERR;
}


void Render::DrawCars(ShaderProg &shader)
{
    for (Vehicle *car : GetExistingVehicles()) {
        glm::vec3 carPos = ToGlmVec3(car->GetPos());
        glm::mat4 carTrans = glm::mat4(1.0f);
        carTrans = glm::translate(carTrans, carPos);
        carTrans = carTrans * QuatToMatrix(car->GetRotation());

        car->GetVehicleModel()->Draw(shader, carTrans);

        // Draw car wheels
        for (int i = 0; i < 4; i++) {
            glm::mat4 wheelTrans = ToGlmMat4(car->GetWheelTransform(i));
            if (car->IsWheelFlipped(i)) {
                wheelTrans = glm::rotate(wheelTrans, SDL_PI_F, glm::vec3(1.0f, 0.0f, 0.0f));
            }
            car->GetWheelModel()->Draw(shader, wheelTrans);
        }
    }
}


void Render::DrawCheckpoints(ShaderProg &shader, Player *p)
{
    if (World::GetCheckpoints().size() > 0 && World::GetRaceState() != RACE_NONE
            && World::GetRaceState() != RACE_ENDED) {
        Checkpoint &checkpoint = World::GetCheckpoints()[p->raceProgress.checkpointsCollected % World::GetCheckpoints().size()];
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, ToGlmVec3(checkpoint.GetPosition()));
        // TODO: Add variable for checkpoint size
        model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f));
        cubeModel->Draw(shader, model, &windowMat);
    }
}



void Render::LoadShaders()
{
    GLERR;
    // Create shaders
    unsigned int vShader = CreateShaderFromFile("shaders/vertex.glsl", 
                                                 GL_VERTEX_SHADER);
    unsigned int fShader = CreateShaderFromFile("shaders/fragment.glsl", 
                                                 GL_FRAGMENT_SHADER);
    pbrShader = CreateAndLinkShaderProgram(vShader, fShader);

    unsigned int vSkybox = CreateShaderFromFile("shaders/v_skybox.glsl",
                                                GL_VERTEX_SHADER);
    unsigned int fSkybox = CreateShaderFromFile("shaders/f_skybox.glsl",
                                                GL_FRAGMENT_SHADER);
    skyboxShader = CreateAndLinkShaderProgram(vSkybox, fSkybox);

    unsigned int vScreen = CreateShaderFromFile("shaders/v_screen.glsl",
                                                GL_VERTEX_SHADER);
    unsigned int fScreen = CreateShaderFromFile("shaders/f_screen.glsl",
                                                GL_FRAGMENT_SHADER);
    screenShader = CreateAndLinkShaderProgram(vScreen, fScreen);

    unsigned int fScreenRaw = CreateShaderFromFile("shaders/f_screen_raw.glsl",
                                                   GL_FRAGMENT_SHADER);

    rawScreenShader = CreateAndLinkShaderProgram(vScreen, fScreenRaw);

    unsigned int vSimpleDepth = CreateShaderFromFile("shaders/v_simple_depth.glsl", GL_VERTEX_SHADER);
    unsigned int fSimpleDepth = CreateShaderFromFile("shaders/f_simple_depth.glsl", GL_FRAGMENT_SHADER);
    simpleDepthShader = CreateAndLinkShaderProgram(vSimpleDepth, fSimpleDepth);

    unsigned int vText = CreateShaderFromFile("shaders/v_text.glsl", GL_VERTEX_SHADER);
    unsigned int fText = CreateShaderFromFile("shaders/f_text.glsl", GL_FRAGMENT_SHADER);
    textShader = CreateAndLinkShaderProgram(vText, fText);

    unsigned int vUI = CreateShaderFromFile("shaders/v_ui.glsl", GL_VERTEX_SHADER);
    unsigned int fUI = CreateShaderFromFile("shaders/f_ui.glsl", GL_FRAGMENT_SHADER);
    uiShader = CreateAndLinkShaderProgram(vUI, fUI);
    glUseProgram(uiShader.id);
    // Initialise uniform for ui shader
    uiShader.SetVec4((char*)"colourMod", 1.0, 1.0, 1.0, 1.0);

    unsigned int fSamplerArrayTest = CreateShaderFromFile("shaders/f_samplerarray_test.glsl",
                                                          GL_FRAGMENT_SHADER);
    samplerArrayTestShader = CreateAndLinkShaderProgram(vScreen, fSamplerArrayTest);
}


void Render::InitSkybox()
{
    // Create VAO
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    
    // Texture and Materials
    skyboxTex = CreateCubemapFromFiles("data/texture/Lycksele/posx.jpg",
                                       "data/texture/Lycksele/negx.jpg",
                                       "data/texture/Lycksele/posy.jpg",
                                       "data/texture/Lycksele/negy.jpg",
                                       "data/texture/Lycksele/posz.jpg",
                                       "data/texture/Lycksele/negz.jpg");
}


void Render::InitText()
{
    GLERR;
    glUseProgram(textShader.id);
    textShader.SetMat4fv((char*)"projection", glm::value_ptr(uiProj));
    GLERR;
    GLERR;

    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    // Need 6 vertices of 4 floats each for a quad.
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    GLERR;
}


void Render::CreateQuadVAO()
{
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    // Create the UI Quad, which has bottom left at 0, 0 and not -1, -1.
    glGenVertexArrays(1, &uiQuadVAO);
    glGenBuffers(1, &uiQuadVBO);
    glBindVertexArray(uiQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, uiQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uiQuadVertices), &uiQuadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);
}




void Render::DebugGUI()
{
    // Update Camera
    ImGui::Begin("Cool window", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);
    
    bool splitBefore = doSplitScreen;
    ImGui::Checkbox("Splitscreen", &doSplitScreen);
    if (doSplitScreen != splitBefore) {
        UpdatePlayerCamAspectRatios();
    }

    
    bool isFullscreen = SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN;
    //bool isFullscreenBefore = isFullscreen;
    //ImGui::Checkbox("Fullscreen", &isFullscreen);
    //if (isFullscreen != isFullscreenBefore) {
    //    SDL_SetWindowFullscreen(window, isFullscreen);
    //}
    

    const SDL_DisplayMode *currentDisplayMode = SDL_GetWindowFullscreenMode(window);
    const char *windowModes[] = {"Windowed", "Fullscreen", "Exclusive Fullscreen"};
    int currentWindowMode;
    if (!isFullscreen) {
        // Windowed
        currentWindowMode = 0;
    }
    else if (currentDisplayMode == NULL) {
        // Fullscreen
        currentWindowMode = 1;
    } 
    else {
        // Fullscreen exclusive
        currentWindowMode = 2;

    }

    // Index of currently selected window mode. Must click apply to apply it
    static int selectedWindowMode = currentWindowMode;
    int numDisplayModes;
    // Display modes (resolutions) supported by monitor
    // TODO: Just get displayModes once at the start of the game
    SDL_DisplayMode **displayModes = SDL_GetFullscreenDisplayModes(
            SDL_GetDisplayForWindow(window), &numDisplayModes);

    ImGui::Combo("Window Mode", &selectedWindowMode, windowModes, 3);

    // List of display mode names, shown in drop down menu
    char displayModeNames[numDisplayModes][32];
    for (int i = 0; i < numDisplayModes; i++) {
        SDL_snprintf(displayModeNames[i], 32,
                     "%dx%d @ %.1f Hz",
                     displayModes[i]->w,
                     displayModes[i]->h,
                     displayModes[i]->refresh_rate);
    }


    // index of currently selected display mode. Must click apply to apply it
    static int selectedDisplayMode = 0;
    if (selectedWindowMode == 2 
        && displayModes != NULL && numDisplayModes > 0
        && ImGui::BeginCombo("Resolution",
                             displayModeNames[selectedDisplayMode])) 
    {
        for (int i = 0; i < numDisplayModes; i++) {
            const bool isSelected = selectedDisplayMode == i;
            if (ImGui::Selectable(displayModeNames[i], isSelected)) {
                selectedDisplayMode = i;
            }
        }
        ImGui::EndCombo();
    }

    if (ImGui::Button("Apply")) {
        if (selectedWindowMode == 0) {
            SDL_SetWindowFullscreen(window, false);
        } else if (selectedWindowMode == 1) {
            SDL_SetWindowFullscreen(window, true);
            SDL_SetWindowFullscreenMode(window, NULL);
        } else if (selectedWindowMode == 2) {
            SDL_SetWindowFullscreen(window, true);
            SDL_SetWindowFullscreenMode(window, displayModes[selectedDisplayMode]);
        }
    }
    SDL_free(displayModes);
    ImGui::End();
}




void Render::GetPlayerSplitScreenBounds(int playerNum, float *outX, float *outY,
                                       float *outW, float *outH)
{
    float w = screenWidth;
    float h = screenHeight;
    if (doSplitScreen && gNumPlayers >= 2) {
        w = screenWidth / 2.0;
        if (gNumPlayers >= 3) {
            h = screenHeight / 2.0;
        }
    }
    // Bottom left of screen
    float x, y;
    switch (playerNum) {
        case 0: /* Player 1 */
            x = 0;
            y = screenHeight - h;
            break;
        case 1: /* Player 2 */
            x = screenWidth / 2.0;
            y = screenHeight - h;
            break;
        case 2: /* Player 3 */
            x = 0;
            y = 0;
            break;
        case 3: /* Player 4 */
            x = screenWidth / 2.0;
            y = 0;
            break;
    }

    *outX = x;
    *outY = y;
    *outW = w;
    *outH = h;
}










void Render::RenderSceneSplitScreen()
{
    GLERR;
    glUseProgram(pbrShader.id);
    GLERR;
    float playerScreenWidth, playerScreenHeight, xOffset, yOffset;
    for (int i = 0; i < gNumPlayers; i++) {
        GetPlayerSplitScreenBounds(i, &xOffset, &yOffset, 
                                   &playerScreenWidth, &playerScreenHeight);
        GLERR;
        //SDL_Log("%d, %d", playerScreenWidth, playerScreenHeight);
        glViewport(xOffset, yOffset, playerScreenWidth, playerScreenHeight);
        GLERR;
        Render::RenderScene(gPlayers[i].cam.cam, &gPlayers[i]);
        GLERR;
        // Only render player 1 if doSplitScreen is off.
        if (!doSplitScreen) break;
    }
                                   
}


void Render::UpdateWindowSize()
{
    bool screenSuccess = SDL_GetWindowSize(window, &screenWidth, &screenHeight);
    if (screenSuccess) {
        glViewport(0, 0, screenWidth, screenHeight);
        // Update projection uniforms in shaders.
        uiProj = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight);
        glUseProgram(uiShader.id);
        uiShader.SetMat4fv((char*)"proj", glm::value_ptr(uiProj));
        glUseProgram(textShader.id);
        textShader.SetMat4fv((char*)"projection", glm::value_ptr(uiProj));
        // Delete and recreate screen frame buffer objects with new screen
        // size.
        CreateFBOs();
    }
    else {
        SDL_Log("Could not get screen size.");
    }
}


void Render::RenderShadowDepthToScreen()
{
    glViewport(0, 0, screenWidth, screenHeight);
    glClearColor(0.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    glUseProgram(samplerArrayTestShader.id);
    glActiveTexture(GL_TEXTURE9);
    glBindTexture(GL_TEXTURE_2D, Render::GetSpotShadowTexAtlas());
    //glBindTexture(GL_TEXTURE_2D, Render::GetTestShadowTex());
    samplerArrayTestShader.SetInt((char*)"spotLightShadowMapAtlas", 9);
    //samplerArrayTestShader.SetInt((char*)"tex", 0);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    GLERR;
}


void Render::ToggleFullscreen()
{
    SDL_SetWindowFullscreen(
            window, !(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN));
    UpdatePlayerCamAspectRatios();
}

Rect Render::ScreenBoundary()
{
    return {0, 0, (float) screenWidth, (float) screenHeight};
}
