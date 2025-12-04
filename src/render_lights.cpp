#include "render_lights.h"
#include "render_defines.h"
#include "render_shaders.h"
#include "render_shadow.h"
#include "shader.h"
#include "player.h"
#include "convert.h"
#include "vehicle.h"
#include "glerr.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <algorithm>

static std::vector<Render::SpotLight*> spotLights;


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
    if (s1 == nullptr || !s1->mEnableShadows) return false;
    if (s2 == nullptr || !s2->mEnableShadows) return true;
    return LengthSquared(s1->mPosition - spotLightDistCompTargetPos) 
        < LengthSquared(s2->mPosition - spotLightDistCompTargetPos);
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


void Render::ResetSpotLightsGPU()
{
    glUseProgram(pbrShader.id);
    char uniformName[64];
    glm::vec3 zero = glm::vec3(0, 0, 0);
    for (int i = 0; i < MAX_SPOT_LIGHTS; i++) {
        SDL_snprintf(uniformName, 64, "spotLights[%d].diffuse", i);
        pbrShader.SetVec3(uniformName, glm::value_ptr(zero));
        SDL_snprintf(uniformName, 64, "spotLights[%d].specular", i);
        pbrShader.SetVec3(uniformName, glm::value_ptr(zero));
        SDL_snprintf(uniformName, 64, "spotLights[%d].position", i);
        pbrShader.SetVec3(uniformName, glm::value_ptr(zero));
        SDL_snprintf(uniformName, 64, "spotLights[%d].direction", i);
        pbrShader.SetVec3(uniformName, glm::value_ptr(zero));
        SDL_snprintf(uniformName, 64, "spotLights[%d].cutoffInner", i);
        pbrShader.SetFloat(uniformName, 0.0);
        SDL_snprintf(uniformName, 64, "spotLights[%d].cutoffOuter", i);
        pbrShader.SetFloat(uniformName, 0.0);
        SDL_snprintf(uniformName, 64, "spotLights[%d].shadowMapIdx", i);
        pbrShader.SetInt(uniformName, -1);
        GLERR;
    }
}


void Render::SetSpotLightUniforms(ShaderProg pbrShader, glm::mat4 view)
{
    char uniformName[64];
    int spotLightNum = 0;
    for (size_t i = 0; i < spotLights.size() && spotLightNum < MAX_SPOT_LIGHTS; i++) {
        if (spotLights[i] == nullptr) continue;
        
        glActiveTexture(GL_TEXTURE0);

        //if (!spotLights[i]->mEnableShadows) continue;
        glm::vec3 lightCol = spotLights[i]->mColour / glm::vec3(1.0);
        //glm::vec3 lightCol = glm::vec3(6000.0);
        SDL_snprintf(uniformName, 64, "spotLights[%d].diffuse", spotLightNum);
        pbrShader.SetVec3(uniformName, glm::value_ptr(lightCol));
        SDL_snprintf(uniformName, 64, "spotLights[%d].specular", spotLightNum);
        pbrShader.SetVec3(uniformName, glm::value_ptr(lightCol));

        glm::vec3 viewPos = glm::vec3(view * glm::vec4(spotLights[i]->mPosition, 1.0));
        SDL_snprintf(uniformName, 64, "spotLights[%d].position", spotLightNum);
        pbrShader.SetVec3(uniformName, glm::value_ptr(viewPos));

        glm::vec3 viewDir = glm::vec3(view * glm::vec4(spotLights[i]->mDirection, 0.0));
        SDL_snprintf(uniformName, 64, "spotLights[%d].direction", spotLightNum);
        pbrShader.SetVec3(uniformName, glm::value_ptr(viewDir));

        SDL_snprintf(uniformName, 64, "spotLights[%d].quadratic", spotLightNum);
        pbrShader.SetFloat(uniformName, spotLights[i]->mQuadratic);
        SDL_snprintf(uniformName, 64, "spotLights[%d].cutoffInner", spotLightNum);
        pbrShader.SetFloat(uniformName, spotLights[i]->mCutoffInner);
        SDL_snprintf(uniformName, 64, "spotLights[%d].cutoffOuter", spotLightNum);
        pbrShader.SetFloat(uniformName, spotLights[i]->mCutoffOuter);
        GLERR;

        // Search for the shadow corresponding to this light
        int spotShadowNum = -1;
        spotShadowNum = GetSpotLightShadowNumForLightIdx(spotLightNum);
        // Set the shadow uniform for this light: -1 for no shadow.
        SDL_snprintf(uniformName, 64, "spotLights[%d].shadowMapIdx", spotLightNum);
        pbrShader.SetInt(uniformName, spotShadowNum);

        spotLightNum++;
    }

}


void Render::SortSpotLights(int numSplitScreens)
{
    int endOffset = spotLights.size();
    for (int i = 0; i < numSplitScreens; i++) {
        int beginOffset = SDL_min(spotLights.size(), MAX_SPOT_SHADOWS) 
        //int beginOffset = spotLights.size()
                          * i / numSplitScreens;
        spotLightDistCompTargetPos = ToGlmVec3(
                gPlayers[i].vehicle->GetPos());
        
        std::sort(spotLights.begin() + beginOffset,
                  spotLights.begin() + endOffset,
                  SpotLightDistComp);
    }
}

int Render::GetSpotLightsSize() { return spotLights.size(); }

Render::SpotLight* 
Render::GetSpotLightByIdx(int idx) { return spotLights[idx]; }
