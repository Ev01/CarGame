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
        void Init();
        ~SpotLight();
        glm::vec3 mPosition;
        glm::vec3 mDirection;
        glm::vec3 mColour;

        float mQuadratic = 1.0;
        float mCutoffInner = 0.0;
        float mCutoffOuter = 0.0;
    };

    struct SpotLightShadow
    {
        unsigned int mShadowTex = 0;
        unsigned int mShadowFBO = 0;
        glm::mat4 lightSpaceMatrix;
    };

        

    bool Init();
    Camera& GetCamera();
    void AssimpAddLight(const aiLight *light, const aiNode *node, aiMatrix4x4 transform);
    Render::SpotLight* CreateSpotLight();
    void DestroySpotLight(SpotLight *spotLight);
    //SpotLight& GetSpotLightById(unsigned int id);
    void PhysicsUpdate(double delta);
    void Update(double delta);
    void RenderFrame();
    void RenderScene(const Camera &cam);
    void RenderScene(const glm::mat4 &view, const glm::mat4 &projection,
                     bool enableSkybox = true);
    void HandleEvent(SDL_Event *event);
    void DeleteAllLights();
    /* Returns current aspect ratio of the window */
    float ScreenAspect();
    SDL_Window* GetWindow();
    SDL_GLContext& GetGLContext();
    void CleanUp();


}



