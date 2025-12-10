#include "render_shadow.h"
#include "render_internal.h"
#include "render_lights.h"
#include "render_defines.h"
//#include "render_shaders.h"
#include "render.h" // TODO: Remove this include
#include "glerr.h"
#include "player.h"
#include "shader.h"

#include "../glad/glad.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SDL3/SDL.h>

// TODO: Remove this because it is duplicated in render.cpp
static const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

// For sun light
static unsigned int depthMapFBO;
static unsigned int depthMap; // Depth map texture
static glm::mat4 lightSpaceMatrix;
static const unsigned int SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096;

// For spot lights
static Render::SpotLightShadow spotLightShadows[MAX_SPOT_SHADOWS];
//static unsigned int spotShadowTexArray;
static unsigned int spotShadowTexAtlas;
static unsigned int spotShadowFBO;

// TODO: rename to cSpotShadowSize
static constexpr unsigned int SPOT_SHADOW_SIZE = 2048;

static constexpr float cSpotShadowNear = 0.2f;
static constexpr float cSpotShadowFar = 40.0f;

//unsigned int testShadowFBO;
//unsigned int testShadowTex;

void Render::PrepareShadowForLight(int spotShadowNum, int spotLightIdx)
{
    SpotLight *sl = GetSpotLightByIdx(spotLightIdx);
    SpotLightShadow &spotShadow = spotLightShadows[spotShadowNum];

    glm::mat4 lightView = glm::lookAt(
            sl->mPosition, sl->mPosition + sl->mDirection, up);
    // The projection that will be used to render the scene from the light's
    // perspective
    glm::mat4 lightProjection = glm::perspective(SDL_acosf(sl->mCutoffOuter) * 2.0f,
                                       1.0f, cSpotShadowNear, cSpotShadowFar);
    spotShadow.lightSpaceMatrix = lightProjection * lightView;
    // Store which light this shadow is for so that when rendering lights later
    // on, they will use the correct shadows.
    spotShadow.mForLightIdx = spotLightIdx; // Render the scene onto the shadow framebuffer
    glActiveTexture(GL_TEXTURE9);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, spotShadowFBO);
    // Individual shadow textures are next to each other in the texture atlas.
    float x = SPOT_SHADOW_SIZE * spotShadowNum;
    glViewport(x, 0, SPOT_SHADOW_SIZE, SPOT_SHADOW_SIZE);
    //glClear(GL_DEPTH_BUFFER_BIT);
    RenderSceneShadow(spotShadow.lightSpaceMatrix);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GLERR;
}


void Render::CreateShadowFBO(unsigned int *inFBO, unsigned int *inTex,
        unsigned int resW, unsigned int resH, float defaultLighting)
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


void Render::CreateShadowFBOForExistingTex(unsigned int *outFBO, unsigned int tex)
{
    glGenFramebuffers(1, outFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, *outFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           tex, 0);
    // Don't draw colours onto this buffer
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        SDL_Log("Error: Framebuffer is not complete!");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}



void Render::CreateShadowTextureArray(unsigned int *outTex,
        unsigned int resW, unsigned int resH, float defaultLighting)
{
    glGenTextures(1, outTex);
    GLERR;
    glBindTexture(GL_TEXTURE_2D_ARRAY, *outTex);

    GLERR;
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, resW, resH,
                 MAX_SPOT_SHADOWS, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    GLERR;
    /*
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    */
    GLERR;
    //float borderColour[] = {defaultLighting, defaultLighting, 
    //                        defaultLighting, defaultLighting};
    //glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColour);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    GLERR;
}


void Render::CreateShadowTextureAtlas(unsigned int *outTex)
{
    const unsigned int resW = SPOT_SHADOW_SIZE * MAX_SPOT_SHADOWS;
    const unsigned int resH = SPOT_SHADOW_SIZE;
    const float defaultLighting = 1.0f;

    glGenTextures(1, outTex);
    glBindTexture(GL_TEXTURE_2D, *outTex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, resW, resH, 0, 
                 GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColour[] = {defaultLighting, defaultLighting, 
                            defaultLighting, defaultLighting};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColour);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void Render::CreateShadowFBOForTexLayer(unsigned int *outFBO, unsigned int tex, int layer)
{
    glGenFramebuffers(1, outFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, *outFBO);
    GLERR;
    /*
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           *inTex, 0);
    */
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, 
                           tex, 0, layer);
    GLERR;
    // Don't draw colours onto this buffer
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        SDL_Log("Error: Framebuffer is not complete!");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GLERR;
}


void Render::ShadowPass()
{
    // For sunlight shadows
    float nearPlane = 1.0f, farPlane = 140.0f;
    // TODO: Make sunlight shadow work in splitscreen
    const glm::vec3 shadowOrigin = gPlayers[0].cam.cam.pos;
    constexpr float SHADOW_START_FAC = 70.0f;
    glm::mat4 lightView = glm::lookAt(
            shadowOrigin - GetSunLight().mDirection * SHADOW_START_FAC,
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
    int i = 0;
    glBindFramebuffer(GL_FRAMEBUFFER, spotShadowFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    for (; shadowNum < MAX_SPOT_SHADOWS && i < GetSpotLightsSize(); i++) {
        SpotLight* sl = GetSpotLightByIdx(i);
        if (sl == nullptr || !sl->mEnableShadows) continue;
        PrepareShadowForLight(shadowNum, i);
        shadowNum++;
    }
    // If there are left over shadows not in use, set their light idx to -1.
    for (; shadowNum < MAX_SPOT_SHADOWS; shadowNum++) {
        spotLightShadows[shadowNum].mForLightIdx = -1;
    }

    // Test shadow
    /*
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, spotShadowFBO);
    glViewport(0, 0, SPOT_SHADOW_SIZE, SPOT_SHADOW_SIZE);
    glClear(GL_DEPTH_BUFFER_BIT);
    RenderSceneShadow(spotLightShadows[0].lightSpaceMatrix);
    */
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    glCullFace(GL_BACK);

    GLERR;
}


/* Render the depth of the light's perspective on the currently bound FBO */
void Render::RenderSceneShadow(glm::mat4 aLightSpaceMatrix)
{
    GLERR;
    glUseProgram(simpleDepthShader.id);
    GLERR;
    simpleDepthShader.SetMat4fv((char*)"lightSpaceMatrix", glm::value_ptr(aLightSpaceMatrix));
    GLERR;
    RenderSceneRaw(simpleDepthShader);
    GLERR;
}


void Render::SetSunShadowUniforms(ShaderProg pbrShader)
{
    pbrShader.SetMat4fv((char*)"lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));
    // Sun shadow map is on texture8
    glActiveTexture(GL_TEXTURE8);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    pbrShader.SetInt((char*)"shadowMap", 8);
    glActiveTexture(GL_TEXTURE0);
    GLERR;
}


void Render::SetSpotShadowUniforms(ShaderProg pbrShader)
{
    char uniformName[64];
    for (int shadowNum = 0; shadowNum < MAX_SPOT_SHADOWS; shadowNum++) {
        //if (spotLightShadows[i].mForLightIdx == -1) continue;
        //SDL_snprintf(uniformName, 64, "spotLightShadowMaps[%d]", shadowNum);
        //pbrShader.SetInt(uniformName, 9 + shadowNum);
        SDL_snprintf(uniformName, 64, "spotLightSpaceMatrix[%d]", shadowNum);
        GLERR;
        pbrShader.SetMat4fv(uniformName, glm::value_ptr(spotLightShadows[shadowNum].lightSpaceMatrix));
        //glActiveTexture(GL_TEXTURE9 + shadowNum);
        //glBindTexture(GL_TEXTURE_2D, spotLightShadows[shadowNum].mShadowTex);
    }
    GLERR;
    glActiveTexture(GL_TEXTURE9);
    glBindTexture(GL_TEXTURE_2D, spotShadowTexAtlas);
    //glBindTexture(GL_TEXTURE_2D_ARRAY, GetSpotShadowTexArray());
    GLERR;
    SDL_snprintf(uniformName, 64, "spotLightShadowMapAtlas");
    pbrShader.SetInt(uniformName, 9);
    //pbrShader.SetInt((char*)"shadowSize", SPOT_SHADOW_SIZE);
    GLERR;
}


void Render::InitShadows()
{
    // Shadow Map
    GLERR;
    //CreateShadowTextureArray(&spotShadowTexArray, SPOT_SHADOW_SIZE, SPOT_SHADOW_SIZE, 1.0f);
    CreateShadowTextureAtlas(&spotShadowTexAtlas);
    GLERR;
    CreateShadowFBOForExistingTex(&spotShadowFBO, spotShadowTexAtlas);
    //SDL_Log("spotShadowTexArray = %d", spotShadowTexArray);
    GLERR;
    // Sun shadow FBO
    CreateShadowFBO(&depthMapFBO, &depthMap, SHADOW_WIDTH, SHADOW_HEIGHT, 1.0f);
    SDL_Log("spotShadowTexAtlas = %d", spotShadowTexAtlas);
    SDL_Log("spotShadowFBO = %d", spotShadowFBO);
    GLERR;
    // Spotlight shadows
    /*
    for (int i = 0; i < MAX_SPOT_SHADOWS; i++) {
        //CreateShadowFBOForTexLayer(&(spotLightShadows[i].mShadowFBO), spotShadowTexArray, i);
        //SDL_Log("Shadow FBO %d = %d", i, spotLightShadows[i].mShadowFBO);
    }
    */

    //CreateShadowFBO(&testShadowFBO, &testShadowTex, SPOT_SHADOW_SIZE, SPOT_SHADOW_SIZE, 1.0f);
    //CreateShadowFBOForTexLayer(&testShadowFBO, spotShadowTexArray, 0);
}

int Render::GetSpotLightShadowNumForLightIdx(int i)
{
    for (int j=0; j < MAX_SPOT_SHADOWS; j++) {
       if (spotLightShadows[j].mForLightIdx == (int)i) {
           return j;
       }
    }
    return -1;
}

//unsigned int Render::GetSpotShadowTexArray()    { return spotShadowTexArray; }
unsigned int Render::GetSpotShadowTexAtlas()    { return spotShadowTexAtlas; }
//unsigned int Render::GetTestShadowTex()         { return testShadowTex; }
