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

    bool Init();
    void AssimpAddLight(const aiLight *light, const aiNode *node, aiMatrix4x4 transform);
    void RenderFrame(const Camera &cam, const Model &mapModel,
                     const Model &carModel, const Model &wheelModel);
    SDL_Window* Window();


}



