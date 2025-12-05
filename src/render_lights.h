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

    /* Resets the spotlight shader uniforms. */
    void ResetSpotLightsGPU();
    /* Sets all uniforms for the spotlights in the PBR shader to keep them
     * synced with the array of spotlights on the CPU. View is the camera's
     * view matrix. This function should be called if the camera's view
     * changes, a light changes, or a light's shadow is changed. Should
     * probably call it every frame. */
    void SetSpotLightUniforms(ShaderProg pbrShader, glm::mat4 view);
    /* Sorts the spotlights from closest to the player to farthest from the
     * player. This is used for giving shadows to the spotlights closest to the
     * player. */
    void SortSpotLights(int numSplitScreens);
    /* Get the size of the spotlights array. This is not the same as the number
     * of spotlights because some elements in the array are nullptr. */
    int GetSpotLightsSize();
    /* Get a spotlight by index in the array. May return nullptr. */
    Render::SpotLight* GetSpotLightByIdx(int idx);
}
