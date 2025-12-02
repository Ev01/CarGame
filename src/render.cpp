#include "render.h"

#include "convert.h"
#include "camera.h"
#include "input.h"
#include "model.h"
#include "shader.h"
#include "../glad/glad.h"
#include "physics.h"
#include "vehicle.h"
#include "world.h"
#include "player.h"
#include "font.h"
#include "main_game.h"
#include "glerr.h"

#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui/backends/imgui_impl_opengl3.h"
#include "../vendor/imgui/backends/imgui_impl_sdl3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <SDL3/SDL.h>
#include <algorithm>


// These must align with defines in shader
#define MAX_SPOT_SHADOWS 8
#define MAX_SPOT_LIGHTS 32

static std::vector<Render::Light> lights;
static std::vector<Render::SpotLight*> spotLights;
static Render::SpotLightShadow spotLightShadows[MAX_SPOT_SHADOWS];
static Render::SunLight sunLight;
static ShaderProg PbrShader;
static ShaderProg skyboxShader;
static ShaderProg screenShader;
static ShaderProg rawScreenShader;
static ShaderProg simpleDepthShader;
static ShaderProg textShader;
static ShaderProg uiShader;
static SDL_Window *window;
static SDL_GLContext context;
int screenWidth, screenHeight;

//static Texture textTex;
static Texture skyboxTex;
static Texture grassTex;
static Texture tachoMeterTex;
static Texture tachoNeedleTex;

static Material grassMat;
static Material windowMat;

static Model *cubeModel;
static Model *quadModel;
//static Camera cam;
static VehicleCamera cam2;
static VehicleCamera cam3;
//static int currentCamNum = 0;
static bool doSplitScreen = false;

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
    -1.0f, 1.0f,  0.0f, 1.0f,     // Top left
    1.0f, 1.0f,    1.0f, 1.0f,     // Top right
    1.0f, -1.0f,   1.0f, 0.0f,    // Bottom right
    1.0f, -1.0f,   1.0f, 0.0f,    // Bottom right
    -1.0f, -1.0f, 0.0f, 0.0f,    // Bottom left
    -1.0f, 1.0f,  0.0f, 1.0f      // Top left
};


static unsigned int skyboxVAO;
static unsigned int skyboxVBO;

static unsigned int fbo;
static unsigned int rbo;
static unsigned int textureColourBuffer;
static unsigned int msFBO;
static unsigned int msRBO;
static unsigned int msTexColourBuffer;

static unsigned int depthMapFBO;
static unsigned int depthMap; // Depth map texture
const unsigned int SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096;
constexpr unsigned int SPOT_SHADOW_SIZE = 4096;
static glm::mat4 lightSpaceMatrix;

static unsigned int quadVAO;
static unsigned int quadVBO;

static const int fbWidth = 1600;
static const int fbHeight = 900;
//static const int fbWidth = 800;
//static const int fbHeight = 600;
static unsigned int physFrameCounter = 0;

//std::map<char, Character> characters;
Character characters[96];
static glm::mat4 uiProj;
unsigned int textVAO, textVBO;


//static const glm::vec3 sunDir = glm::vec3(0.1, -0.5, 0.1);
static const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);


static void CreateFramebuffer(unsigned int *aFBO, unsigned int *aCbTex, unsigned int *aRBO)
{
    glGenFramebuffers(1, aFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, *aFBO);

    // Create texture for frame buffer
    glGenTextures(1, aCbTex);
    glBindTexture(GL_TEXTURE_2D, *aCbTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, fbWidth, fbHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
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
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, fbWidth, fbHeight);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    // Attach render buffer to framebuffer
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, *aRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        SDL_Log("Error: Framebuffer is not complete!");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


static void CreateMSFramebuffer(unsigned int *aFBO, unsigned int *aCbTex, unsigned int *aRBO)
{
    glGenFramebuffers(1, aFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, *aFBO);

    // Create texture for frame buffer
    glGenTextures(1, aCbTex);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, *aCbTex);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB32F, fbWidth, fbHeight, GL_TRUE);
    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    // Attach texture to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, *aCbTex, 0);
    // Create renderbuffer for depth and stencil components
    glGenRenderbuffers(1, aRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, *aRBO);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, fbWidth, fbHeight);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    // Attach render buffer to framebuffer
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, *aRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        SDL_Log("Error: Framebuffer is not complete!");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


static void RenderSkybox(glm::mat4 view, glm::mat4 projection)
{
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    // Remove translation component from view matrix
    glm::mat4 skyboxView = glm::mat4(glm::mat3(view));
    glUseProgram(skyboxShader.id);

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


static void DrawMap(ShaderProg &shader)
{
    Model &mapModel = World::GetCurrentMapModel();
    glm::mat4 model = glm::mat4(1.0f);
    shader.SetMat4fv((char*)"model", glm::value_ptr(model));
    mapModel.Draw(shader);
}


static void DrawCars(ShaderProg &shader)
{
    for (Vehicle *car : GetExistingVehicles()) {
        glm::vec3 carPos = ToGlmVec3(car->GetPos());
        glm::mat4 carTrans = glm::mat4(1.0f);
        carTrans = glm::translate(carTrans, carPos);
        carTrans = carTrans * QuatToMatrix(car->GetRotation());

        car->mVehicleModel->Draw(shader, carTrans);

        // Draw car wheels
        for (int i = 0; i < 4; i++) {
            glm::mat4 wheelTrans = ToGlmMat4(car->GetWheelTransform(i));
            if (car->IsWheelFlipped(i)) {
                wheelTrans = glm::rotate(wheelTrans, SDL_PI_F, glm::vec3(1.0f, 0.0f, 0.0f));
            }
            car->mWheelModel->Draw(shader, wheelTrans);
        }
    }
}


static void DrawCheckpoints(ShaderProg &shader)
{
    if (World::GetCheckpoints().size() > 0 && World::GetRaceState() != RACE_NONE) {
        Checkpoint &checkpoint = World::GetCheckpoints()[gPlayerRaceProgress.mCheckpointsCollected % World::GetCheckpoints().size()];
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, ToGlmVec3(checkpoint.GetPosition()));
        // TODO: Add variable for checkpoint size
        model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f));
        cubeModel->Draw(shader, model, &windowMat);
    }
}


/*
 * Render the scene without any shader setup
 */
static void RenderSceneRaw(ShaderProg &shader)
{
    // Draw map
    DrawMap(shader);

    // Draw all cars
    DrawCars(shader);

    // Draw next checkpoint in race
    DrawCheckpoints(shader);

}


static void RenderSceneShadow(glm::mat4 aLightSpaceMatrix)
{
    GLERR;
    glUseProgram(simpleDepthShader.id);
    GLERR;
    simpleDepthShader.SetMat4fv((char*)"lightSpaceMatrix", glm::value_ptr(aLightSpaceMatrix));
    GLERR;
    RenderSceneRaw(simpleDepthShader);
    GLERR;
}


static void CreateShadowFBO(unsigned int *inFBO, unsigned int *inTex, unsigned int resW, unsigned int resH, float defaultLighting = 0.0f)
{
    glGenFramebuffers(1, inFBO);
    glGenTextures(1, inTex);

    glBindTexture(GL_TEXTURE_2D, *inTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                 resW, resH, 0, GL_DEPTH_COMPONENT,
                 GL_FLOAT, NULL);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColour[] = {defaultLighting, defaultLighting, 
                            defaultLighting, defaultLighting};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColour);

    glBindFramebuffer(GL_FRAMEBUFFER, *inFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           *inTex, 0);
    // Don't draw colours onto this buffer
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}


static float LengthSquared(const glm::vec3 v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

static glm::vec3 spotLightDistCompTargetPos = glm::vec3(0.0, 0.0, 0.0);
// Returns true if s1 is closer to spotLightDistCompTargetPos than s2.
// This is for use in sort functions such as std::sort. Before using this
// function, don't forget to change spotLightDistCompTargetPos to whatever you
// want to compare spot light distances to (e.g. player position)
static bool SpotLightDistComp(Render::SpotLight *s1, Render::SpotLight *s2) {
    //glm::vec3 playerPos = ToGlmVec3(World::GetCar().GetPos());
    if (s1 == nullptr) return false;
    if (s2 == nullptr) return true;
    return LengthSquared(s1->mPosition - spotLightDistCompTargetPos) 
        < LengthSquared(s2->mPosition - spotLightDistCompTargetPos);
}
/*
static bool SpotLightDistComp2(Render::SpotLight *s1, Render::SpotLight *s2) {
    glm::vec3 playerPos = ToGlmVec3(World::GetCar2().GetPos());
    return LengthSquared(s1->mPosition - playerPos) < LengthSquared(s2->mPosition - playerPos);
}
*/

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


Render::SpotLight* Render::CreateSpotLight()
{
    SpotLight *newSpotLight = new SpotLight;
    newSpotLight->Init();
    for (size_t i = 0; i < spotLights.size(); i++) {
        // Look for null value in spotLights vector
        if (spotLights[i] == nullptr) {
            spotLights[i] = newSpotLight;
            return newSpotLight;
        }
    }
    // If no null value, add to end
    spotLights.push_back(newSpotLight);
    return newSpotLight;
}

void Render::SpotLight::Init()
{
    // Create shadow map for spotlight
    //CreateShadowFBO(&mShadowFBO, &mShadowTex,
    //                SPOT_SHADOW_SIZE, SPOT_SHADOW_SIZE);
}
Render::SpotLight::~SpotLight()
{
    //glDeleteFramebuffers(1, &mShadowFBO);
    //glDeleteTextures(1, &mShadowTex);
}

void Render::DestroySpotLight(SpotLight *spotLight)
{
    for (size_t i = 0; i < spotLights.size(); i++) {
        if (spotLights[i] == spotLight) {
            spotLights[i] = nullptr;
            delete spotLight;
            return;
        }
    }

    SDL_Log("Warning: Called DestroySpotLight, but spot light was not in spotLights vector.");
    delete spotLight;
}


static bool LoadFont()
{
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        SDL_Log("Could not init FreeType Library");
        return false;
    }

    FT_Face face;
    if (FT_New_Face(ft, "data/fonts/liberation_sans/LiberationSans-Regular.ttf", 0, &face)) {
        SDL_Log("Failed to load font");
    }

    FT_Set_Pixel_Sizes(face, 0, 48);
    if (FT_Load_Char(face, 'X', FT_LOAD_RENDER)) {
        SDL_Log("Failed to load glyph");
    }

    GLERR;
    // disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    // Loop through first 128 ascii characters (0-31 are control characters)
    for (unsigned char c = 32; c < 128; c++) {
        // load character glyph
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            SDL_Log("Failed to load glyph");
        }
        Texture tex;
        glGenTextures(1, &tex.id);
        glBindTexture(GL_TEXTURE_2D, tex.id);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GLERR;
        // Store character for later
        Character character = {
            tex,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left,  face->glyph->bitmap_top),
            static_cast<unsigned int>(face->glyph->advance.x)
        };
        characters[c - 32] = character;
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

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
    return true;
}


static void LoadShaders()
{
    GLERR;
    // Create shaders
    unsigned int vShader = CreateShaderFromFile("shaders/vertex.glsl", 
                                                 GL_VERTEX_SHADER);
    unsigned int fShader = CreateShaderFromFile("shaders/fragment.glsl", 
                                                 GL_FRAGMENT_SHADER);
    PbrShader = CreateAndLinkShaderProgram(vShader, fShader);

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
}


static void InitSkybox()
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


static void CreateQuadVAO()
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
    if (!LoadFont()) {
        return false;
    }

    GLERR;
    // ----- Framebuffer -----
    CreateFramebuffer(&fbo, &textureColourBuffer, &rbo);
    GLERR;
    CreateMSFramebuffer(&msFBO, &msTexColourBuffer, &msRBO);

    // Shadow Map
    GLERR;
    CreateShadowFBO(&depthMapFBO, &depthMap, SHADOW_WIDTH, SHADOW_HEIGHT, 1.0f);
    // Spotlight shadows
    for (SpotLightShadow &spotShadow : spotLightShadows) {
        CreateShadowFBO(&(spotShadow.mShadowFBO), &(spotShadow.mShadowTex),
                        SPOT_SHADOW_SIZE, SPOT_SHADOW_SIZE);
    }

    GLERR;


    // Enable MSAA (anti-aliasing)
    glEnable(GL_MULTISAMPLE);
    // Enable transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
    // Screen width and height
    int sw, sh;
    bool screenSuccess = SDL_GetWindowSize(window, &sw, &sh);
    if (screenSuccess) {
        float aspect = (float) sw / (float) sh;
        // Divide aspect by 2.0 for split screen
        if (doSplitScreen && gNumPlayers == 2) {
            aspect /= 2.0;
        }
        return aspect;
    }
    return 0.0;
}


// TODO: Remove this function
Camera& Render::GetCamera()
{
    return gPlayers[0].cam.cam;
}


void Render::PhysicsUpdate(double delta)
{
    if (physFrameCounter % 30 == 0) {
        // Sort spot lights from nearest to player to furthest from player.
        // This doesn't need to happen very often.
        
        //int numSplitScreens = doSplitScreen && gNumPlayers >= 2 ? 2 : 1;
        int numSplitScreens = doSplitScreen ? gNumPlayers : 1;
        //int endOffset = SDL_min(spotLights.size(), MAX_SPOT_SHADOWS);
        int endOffset = spotLights.size();
        for (int i = 0; i < numSplitScreens; i++) {
            //int beginOffset = SDL_min(spotLights.size(), MAX_SPOT_SHADOWS) 
            int beginOffset = spotLights.size()
                              * i / numSplitScreens;
            spotLightDistCompTargetPos = ToGlmVec3(
                    gPlayers[i].vehicle->GetPos());
            
            std::sort(spotLights.begin() + beginOffset,
                      spotLights.begin() + endOffset,
                      SpotLightDistComp);
            
        }

        /*
        spotLightDistCompTargetPos = ToGlmVec3(gPlayers[0].vehicle->GetPos());
        std::sort(spotLights.begin(), spotLights.end(), SpotLightDistComp);
        if (doSplitScreen && gNumPlayers >= 2) {
          spotLightDistCompTargetPos = ToGlmVec3(gPlayers[1].vehicle->GetPos());
          int endOffset = SDL_min(spotLights.size() / 2, MAX_SPOT_SHADOWS / 2);
          std::sort(spotLights.begin() + endOffset, spotLights.end(),
                    SpotLightDistComp);
        }
        */
    }

    physFrameCounter++;
}


static void UpdatePlayerCamAspectRatios()
{
        float aspect = Render::ScreenAspect();
        for (int i = 0; i < gNumPlayers; i++) {
            gPlayers[i].cam.cam.SetAspectRatio(aspect);
        }
}


static void DebugGUI()
{
    // Update Camera
    ImGui::Begin("Cool window");
    
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
    ImGui::End();
}


void Render::Update(double delta)
{
    DebugGUI();
}


static void PrepareShadowForLight(int shadowIdx, int spotLightIdx)
{
    glm::mat4 lightView = glm::lookAt(
            spotLights[spotLightIdx]->mPosition,
            spotLights[spotLightIdx]->mPosition + spotLights[spotLightIdx]->mDirection,
            up);
    // TODO: clean up and optimize
    const float nearPlane = 0.2f;
    const float farPlane = 40.0f;
    // The projection that will be used to render the scene from the light's
    // perspective
    glm::mat4 lightProjection = glm::perspective(SDL_acosf(spotLights[spotLightIdx]->mCutoffOuter) * 2.0f,
                                       1.0f, nearPlane, farPlane);
    spotLightShadows[shadowIdx].lightSpaceMatrix = lightProjection * lightView;
    // Store which light this shadow is for so that when rendering lights later
    // on, they will use the correct shadows.
    spotLightShadows[shadowIdx].mForLightIdx = spotLightIdx;
    // Render the scene onto the shadow framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, spotLightShadows[shadowIdx].mShadowFBO);
    glViewport(0, 0, SPOT_SHADOW_SIZE, SPOT_SHADOW_SIZE);
    glClear(GL_DEPTH_BUFFER_BIT);
    RenderSceneShadow(spotLightShadows[shadowIdx].lightSpaceMatrix);
}


static void ShadowPass()
{
    // For sunlight shadows
    float nearPlane = 1.0f, farPlane = 140.0f;
    // TODO: Make sunlight shadow work in splitscreen
    const glm::vec3 shadowOrigin = gPlayers[0].cam.cam.pos;
    constexpr float SHADOW_START_FAC = 70.0f;
    glm::mat4 lightView = glm::lookAt(
            shadowOrigin - sunLight.mDirection * SHADOW_START_FAC,
            shadowOrigin, up);
    glm::mat4 lightProjection = glm::ortho(-40.0f, 40.0f, -40.0f, 40.0f, 
                                    nearPlane, farPlane);

    // Transforms world space to light space
    lightSpaceMatrix = lightProjection * lightView;

    
    glEnable(GL_CULL_FACE);
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);
    glCullFace(GL_FRONT);
    GLERR;
    // For sun shadow
    RenderSceneShadow(lightSpaceMatrix);

    // Render shadows for some spotlights
    int shadowNum = 0;
    size_t i = 0;
    for (; shadowNum < MAX_SPOT_SHADOWS && i < spotLights.size(); i++) {
        if (spotLights[i] == nullptr || !spotLights[i]->mEnableShadows) continue;
        PrepareShadowForLight(shadowNum, i);
        shadowNum++;
    }
    // If there are left over shadows not in use, set their light idx to -1.
    for (; shadowNum < MAX_SPOT_SHADOWS; shadowNum++) {
        spotLightShadows[shadowNum].mForLightIdx = -1;
    }
    glCullFace(GL_BACK);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    
    GLERR;
}

enum UIAnchor {
    UI_ANCHOR_BOTTOM_LEFT  = 0b00,
    UI_ANCHOR_TOP_LEFT     = 0b01,
    UI_ANCHOR_BOTTOM_RIGHT = 0b10,
    UI_ANCHOR_TOP_RIGHT    = 0b11,
};

static void RenderUIAnchored(Texture tex, glm::vec2 scale, glm::vec2 margin,
                             float rotation, UIAnchor anchor,
                             float boundX=0.0, float boundY=0.0,
                             float boundW=0.0, float boundH=0.0)
{
    glm::mat4 trans = glm::mat4(1.0);
    glm::vec2 pos;
    //float boundW  = (float)screenWidth;
    //float boundH = (float)screenHeight;
    if (boundW == 0.0) boundW = (float)screenWidth;
    if (boundH == 0.0) boundH = (float)screenHeight;
    float boundR = boundX + boundW; /* right boundary */
    float boundT = boundY + boundH; /* top boundary   */
    // Divide scale because the quad is originally 2 wide and 2 high.
    scale = scale / 2.0f;
    switch (anchor) {
        case UI_ANCHOR_BOTTOM_LEFT:
            pos = glm::vec2(boundX + margin.x + scale.x,
                            boundY + margin.y + scale.y);
            break;
        case UI_ANCHOR_TOP_LEFT:
            pos = glm::vec2(boundX + margin.x + scale.x,
                            boundT + margin.y - scale.y);
            break;
        case UI_ANCHOR_BOTTOM_RIGHT:
            pos = glm::vec2(boundR + margin.x - scale.x,
                            boundY + margin.y + scale.y);
            break;
        case UI_ANCHOR_TOP_RIGHT:
            pos = glm::vec2(boundR  + margin.x - scale.x,
                            boundT + margin.y - scale.y);
            break;
    }
    trans = glm::translate(trans, glm::vec3(pos.x, pos.y, 0.0f));
    trans = glm::rotate(trans, rotation, glm::vec3(0, 0, 1));
    trans = glm::scale(trans, glm::vec3(scale.x, scale.y, 1.0f));

    glViewport(0, 0, screenWidth, screenHeight);

    // Draw the texture
    glUseProgram(uiShader.id);
    uiShader.SetMat4fv((char*)"trans", glm::value_ptr(trans));
    uiShader.SetMat4fv((char*)"proj", glm::value_ptr(uiProj));
    glBindVertexArray(quadVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex.id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}


static void GetPlayerSplitScreenBounds(int playerNum, int *outX, int *outY,
                                       int *outW, int *outH)
{
    int w = screenWidth;
    int h = screenHeight;
    if (doSplitScreen && gNumPlayers >= 2) {
        w = screenWidth / 2;
        if (gNumPlayers >= 3) {
            h = screenHeight / 2;
        }
    }
    // Bottom left of screen
    int x, y;
    switch (playerNum) {
        case 0: /* Player 1 */
            x = 0;
            y = screenHeight - h;
            break;
        case 1: /* Player 2 */
            x = screenWidth / 2;
            y = screenHeight - h;
            break;
        case 2: /* Player 3 */
            x = 0;
            y = 0;
            break;
        case 3: /* Player 4 */
            x = screenWidth / 2;
            y = 0;
            break;
    }

    *outX = x;
    *outY = y;
    *outW = w;
    *outH = h;
}


static void RenderPlayerTachometer(int playerNum)
{
    int boundX, boundY, boundW, boundH;
    GetPlayerSplitScreenBounds(playerNum, &boundX, &boundY, &boundW, &boundH);
    RenderUIAnchored(tachoMeterTex, glm::vec2(256.0f, 256.0f),
                     glm::vec2(-32.0f, 32.0f), 0.0, UI_ANCHOR_BOTTOM_RIGHT,
                     boundX, boundY, boundW, boundH);

    // Draw the needle
    Player &p = gPlayers[playerNum];
    if (p.vehicle != NULL) {
        float rpm = p.vehicle->GetEngineRPM();
        // Change these constants based on the tachometer graphic.
        const float zeroAngle = 0.74;
        const float angleRange = 4.28;
        const float rpmRange = 8000.0;
        float needleAngle = zeroAngle - rpm / rpmRange * angleRange;
        RenderUIAnchored(
                tachoNeedleTex, glm::vec2(256.0f, 256.0f),
                glm::vec2(-32.0f, 32.0f), needleAngle, UI_ANCHOR_BOTTOM_RIGHT,
                boundX, boundY, boundW, boundH);
    }
}




static void GuiPass()
{
    if (MainGame::gGameState == GAME_PRESS_START_SCREEN) {
        Render::RenderText(textShader, "Hello!!! This is a sentence.",
                           25.0f, 25.0f, 1.0f, glm::vec3(0.5, 0.8f, 0.2f));
        Render::RenderText(textShader, "Press Enter and up arrow to start...", 
                           100.0f, 100.0f, 0.5f, glm::vec3(0.0, 0.0f, 0.0f));
    }

    // Render the tachometer for all players
    for (int i = 0; i < gNumPlayers; i++) {
        RenderPlayerTachometer(i);
        // Only render player 1 if doSplitScreen is off.
        if (!doSplitScreen) break;
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}




static void RenderSceneSplitScreen()
{
    glUseProgram(PbrShader.id);
    int playerScreenWidth, playerScreenHeight, xOffset, yOffset;
    for (int i = 0; i < gNumPlayers; i++) {
        GetPlayerSplitScreenBounds(i, &xOffset, &yOffset, 
                                   &playerScreenWidth, &playerScreenHeight);
        glViewport(xOffset, yOffset, playerScreenWidth, playerScreenHeight);
        Render::RenderScene(gPlayers[i].cam.cam);
        // Only render player 1 if doSplitScreen is off.
        if (!doSplitScreen) break;
    }
                                   
}


static void UpdateWindowSize()
{
    bool screenSuccess = SDL_GetWindowSize(window, &screenWidth, &screenHeight);
    if (screenSuccess) {
        glViewport(0, 0, screenWidth, screenHeight);
        uiProj = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight);
    }
    else {
        SDL_Log("Could not get screen size.");
    }
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
    
    if (MainGame::gGameState == GAME_IN_WORLD) {
        RenderSceneSplitScreen();
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
    glViewport(0, 0, fbWidth, fbHeight);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    
    glUseProgram(screenShader.id);
    glBindVertexArray(quadVAO);
    glBindTexture(GL_TEXTURE_2D, textureColourBuffer);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Do UI pass afterwards to avoid postprocessing on UI
    GuiPass();
    
    GLERR;

    
    SDL_GL_SwapWindow(window);
}


void Render::RenderScene(const Camera &cam)
{
    RenderScene(cam.LookAtMatrix(up), cam.projection);
}


void Render::ResetSpotLightsGPU()
{
    char uniformName[64];
    glm::vec3 zero = glm::vec3(0, 0, 0);
    for (int i = 0; i < MAX_SPOT_LIGHTS; i++) {
        SDL_snprintf(uniformName, 64, "spotLights[%d].diffuse", i);
        PbrShader.SetVec3(uniformName, glm::value_ptr(zero));
        SDL_snprintf(uniformName, 64, "spotLights[%d].specular", i);
        PbrShader.SetVec3(uniformName, glm::value_ptr(zero));
        SDL_snprintf(uniformName, 64, "spotLights[%d].position", i);
        PbrShader.SetVec3(uniformName, glm::value_ptr(zero));
        SDL_snprintf(uniformName, 64, "spotLights[%d].direction", i);
        PbrShader.SetVec3(uniformName, glm::value_ptr(zero));
        SDL_snprintf(uniformName, 64, "spotLights[%d].cutoffInner", i);
        PbrShader.SetFloat(uniformName, 0.0);
        SDL_snprintf(uniformName, 64, "spotLights[%d].cutoffOuter", i);
        PbrShader.SetFloat(uniformName, 0.0);
        SDL_snprintf(uniformName, 64, "spotLights[%d].shadowMapIdx", i);
        PbrShader.SetInt(uniformName, -1);
        GLERR;
    }
}


void Render::RenderScene(const glm::mat4 &view, const glm::mat4 &projection, 
                         bool enableSkybox)
{
    glEnable(GL_CULL_FACE);

    int windowWidth;
    int windowHeight;
    if (!SDL_GetWindowSize(window, &windowWidth, &windowHeight)) {
        SDL_Log("Could not get window size, using defaults");
        windowWidth = 800;
        windowHeight = 600;
    } 

    ShaderProg &shader = PbrShader;
    glUseProgram(shader.id);

    shader.SetMat4fv((char*)"projection", glm::value_ptr(projection));
    shader.SetMat4fv((char*)"view", glm::value_ptr(view));

    GLERR;

    // Shadow setup
    shader.SetMat4fv((char*)"lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));
    glActiveTexture(GL_TEXTURE8);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    shader.SetInt((char*)"shadowMap", 8);
    glActiveTexture(GL_TEXTURE0);
    GLERR;

    glm::vec3 sunCol = sunLight.mColour / glm::vec3(1.0);
    // Sunlight direction in view space
    glm::vec3 sunDir = glm::vec3(view * glm::vec4(sunLight.mDirection, 0.0));
    shader.SetVec3((char*)"dirLight.direction", glm::value_ptr(sunDir));
    shader.SetVec3((char*)"dirLight.ambient", 0.05, 0.05, 0.05);
    shader.SetVec3((char*)"dirLight.diffuse", glm::value_ptr(sunCol));
    shader.SetVec3((char*)"dirLight.specular", glm::value_ptr(sunCol));

    GLERR;
    char uniformName[64];
    int lightNum = 0;
    for (size_t i = 0; i < lights.size(); i++) {
        SDL_snprintf(uniformName, 64, "pointLights[%d].position", lightNum);
        glm::vec3 viewPos = glm::vec3(view * glm::vec4(lights[i].mPosition, 1.0));
        shader.SetVec3(uniformName, glm::value_ptr(viewPos));

        glm::vec3 lightCol = lights[i].mColour / glm::vec3(1.0);
        //glm::vec3 lightCol = glm::vec3(6000.0);
        SDL_snprintf(uniformName, 64, "pointLights[%d].ambient", lightNum);
        shader.SetVec3(uniformName, 0.0, 0.0, 0.0);
        SDL_snprintf(uniformName, 64, "pointLights[%d].diffuse", lightNum);
        shader.SetVec3(uniformName, glm::value_ptr(lightCol));
        SDL_snprintf(uniformName, 64, "pointLights[%d].specular", lightNum);
        shader.SetVec3(uniformName, glm::value_ptr(lightCol));

        //SDL_Log("Colour = (%f, %f, %f)", lights[i].mColour.x, lights[i].mColour.y, lights[i].mColour.z);

        SDL_snprintf(uniformName, 64, "pointLights[%d].quadratic", lightNum);
        shader.SetFloat(uniformName, 1.0f);
        SDL_snprintf(uniformName, 64, "pointLights[%d].constant", lightNum);
        shader.SetFloat(uniformName, 0.0f);
        SDL_snprintf(uniformName, 64, "pointLights[%d].linear", lightNum);
        shader.SetFloat(uniformName, 0.0f);
        lightNum++;
    }

    GLERR;
    
    // Set spot light shadow uniforms
    for (int shadowNum = 0; shadowNum < MAX_SPOT_SHADOWS; shadowNum++) {
        //if (spotLightShadows[i].mForLightIdx == -1) continue;
        SDL_snprintf(uniformName, 64, "spotLightShadowMaps[%d]", shadowNum);
        shader.SetInt(uniformName, 9 + shadowNum);
        SDL_snprintf(uniformName, 64, "spotLightSpaceMatrix[%d]", shadowNum);
        shader.SetMat4fv(uniformName, glm::value_ptr(spotLightShadows[shadowNum].lightSpaceMatrix));
        glActiveTexture(GL_TEXTURE9 + shadowNum);
        glBindTexture(GL_TEXTURE_2D, spotLightShadows[shadowNum].mShadowTex);
    }
    
    // Set Spot light uniforms
    int spotLightNum = 0;
    for (size_t i = 0; i < spotLights.size() && spotLightNum < MAX_SPOT_LIGHTS; i++) {
        if (spotLights[i] == nullptr) continue;
        
        glActiveTexture(GL_TEXTURE0);

        //if (!spotLights[i]->mEnableShadows) continue;
        glm::vec3 lightCol = spotLights[i]->mColour / glm::vec3(1.0);
        //glm::vec3 lightCol = glm::vec3(6000.0);
        SDL_snprintf(uniformName, 64, "spotLights[%d].diffuse", spotLightNum);
        shader.SetVec3(uniformName, glm::value_ptr(lightCol));
        SDL_snprintf(uniformName, 64, "spotLights[%d].specular", spotLightNum);
        shader.SetVec3(uniformName, glm::value_ptr(lightCol));

        glm::vec3 viewPos = glm::vec3(view * glm::vec4(spotLights[i]->mPosition, 1.0));
        SDL_snprintf(uniformName, 64, "spotLights[%d].position", spotLightNum);
        shader.SetVec3(uniformName, glm::value_ptr(viewPos));

        glm::vec3 viewDir = glm::vec3(view * glm::vec4(spotLights[i]->mDirection, 0.0));
        SDL_snprintf(uniformName, 64, "spotLights[%d].direction", spotLightNum);
        shader.SetVec3(uniformName, glm::value_ptr(viewDir));

        SDL_snprintf(uniformName, 64, "spotLights[%d].quadratic", spotLightNum);
        shader.SetFloat(uniformName, spotLights[i]->mQuadratic);
        SDL_snprintf(uniformName, 64, "spotLights[%d].cutoffInner", spotLightNum);
        shader.SetFloat(uniformName, spotLights[i]->mCutoffInner);
        SDL_snprintf(uniformName, 64, "spotLights[%d].cutoffOuter", spotLightNum);
        shader.SetFloat(uniformName, spotLights[i]->mCutoffOuter);
        GLERR;

        // Search for the shadow corresponding to this light
        int spotShadowNum = -1;
        for (int j=0; j < MAX_SPOT_SHADOWS; j++) {
           if (spotLightShadows[j].mForLightIdx == (int)i) {
               spotShadowNum = j;
               break;
           }
        }
        // Set the shadow uniform for this light: -1 for no shadow.
        SDL_snprintf(uniformName, 64, "spotLights[%d].shadowMapIdx", spotLightNum);
        shader.SetInt(uniformName, spotShadowNum);

        spotLightNum++;
    }
    //SDL_Log("Num spot lights: %d", spotLightNum);


    //shader.SetFloat((char*)"material.shininess", 32.0f);

    RenderSceneRaw(shader);
    // Draw skybox
    if (enableSkybox) {
        RenderSkybox(view, projection);
    }
    glDisable(GL_CULL_FACE);
}


void Render::RenderText(ShaderProg &s, std::string text, float x, float y,
                        float scale, glm::vec3 colour)
{
    glUseProgram(s.id);
    s.SetVec3((char*)"textColour", colour.x, colour.y, colour.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);

    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) {
        Character ch = characters[*c - 32];

        float xPos = x + ch.bearing.x * scale;
        float yPos = y - (ch.size.y - ch.bearing.y) * scale;
        float w = ch.size.x * scale;
        float h = ch.size.y * scale;
        // Update VBO for each character
        float vertices[6][4] = {
            { xPos,     yPos + h, 0.0f, 0.0f },
            { xPos,     yPos,     0.0f, 1.0f },
            { xPos + w, yPos,     1.0f, 1.0f },

            { xPos,     yPos + h, 0.0f, 0.0f },
            { xPos + w, yPos,     1.0f, 1.0f },
            { xPos + w, yPos + h, 1.0f, 0.0f }
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.texture.id);
        // Update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // Advance cursors for next glyph (advance is in 1/64 pixels)
        x += (ch.advance >> 6) * scale;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}



static void ToggleFullscreen()
{
        SDL_SetWindowFullscreen(
                window, !(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN));
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


