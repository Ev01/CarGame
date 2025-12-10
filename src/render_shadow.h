#pragma once

#include <glm/glm.hpp>

// Forward declarations
struct ShaderProg;

namespace Render {
    //void PrepareShadowForLight(int shadowIdx, int spotLightIdx);
    void PrepareShadowForLight(int spotShadowNum, int spotLightIdx);
    void CreateShadowFBO(unsigned int *inFBO, unsigned int *inTex,
            unsigned int resW, unsigned int resH, float defaultLighting = 0.0f);
    void CreateShadowTextureArray(unsigned int *outTex,
            unsigned int resW, unsigned int resH, float defaultLighting = 0.0f);
    void CreateShadowFBOForTexLayer(unsigned int *outFBO, unsigned int tex, int layer);
    void CreateShadowTextureAtlas(unsigned int *outTex);
    void CreateShadowFBOForExistingTex(unsigned int *outFBO, unsigned int tex);
    void ShadowPass();
    void RenderSceneShadow(glm::mat4 aLightSpaceMatrix);
    void SetSunShadowUniforms(ShaderProg pbrShader);
    void InitShadows();
    unsigned int GetSpotShadowTexArray();    
    void SetSpotShadowUniforms(ShaderProg pbrShader);
    int GetSpotLightShadowNumForLightIdx(int i);
    unsigned int GetSpotShadowTexAtlas();
    //unsigned int GetTestShadowTex();
}
