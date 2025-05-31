#pragma once

#include "camera.h"
#include "model.h"

#include <glm/glm.hpp>
#include <assimp/scene.h>
#include <assimp/light.h>
#include <SDL3/SDL.h>

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
        glm::vec3 mPosition;
        glm::vec3 mDirection;
        glm::vec3 mColour;

        float mQuadratic;
        float mCutoffInner;
        float mCutoffOuter;
    };
        

    bool Init();
    void AssimpAddLight(const aiLight *light, const aiNode *node, aiMatrix4x4 transform);
    void RenderFrame(const Camera &cam, const Model &mapModel,
                     const Model &carModel, const Model &wheelModel);
    void RenderScene(const Camera &cam, const Model &mapModel,
                     const Model &carModel, const Model &wheelModel);
    void HandleEvent(SDL_Event *event);
    SDL_Window* Window();
    void CleanUp();


}



