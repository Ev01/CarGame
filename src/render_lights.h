#pragma once

#include "shader.h"
#include <glm/glm.hpp>

namespace Render {
    struct Light
    {
        glm::vec3 mPosition;
        glm::vec3 mColour;
        float mQuadratic;
    };

    struct SunLight
    {
        glm::vec3 mDirection;
        glm::vec3 mColour;
    };

    struct SpotLight
    {
        void Init();
        ~SpotLight();
        glm::vec3 mPosition;
        glm::vec3 mDirection;
        glm::vec3 mColour;

        float mQuadratic = 1.0;
        float mCutoffInner = 0.0;
        float mCutoffOuter = 0.0;
        bool mEnableShadows = true;
    };

    struct SpotLightShadow
    {
        //unsigned int mShadowTex = 0;
        //unsigned int mShadowFBO = 0;
        int mForLightIdx = -1;
        glm::mat4 lightSpaceMatrix;
    };

    Render::SpotLight* CreateSpotLight();
    void DestroySpotLight(SpotLight *spotLight);
    void ResetSpotLightsGPU();
    void SetSpotLightUniforms(ShaderProg pbrShader, glm::mat4 view);
    void SortSpotLights(int numSplitScreens);

    int GetSpotLightsSize();
    Render::SpotLight* GetSpotLightByIdx(int idx);
}
