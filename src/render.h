#pragma once

#include "camera.h"
#include "render_lights.h"

#include <glm/glm.hpp>
#include <assimp/scene.h>
#include <assimp/light.h>
#include <SDL3/SDL.h>

// forward declarations
struct ShaderProg;

namespace Render {
    bool Init();
    void AssimpAddLight(const aiLight *light, const aiNode *node, aiMatrix4x4 transform);
    void PhysicsUpdate(double delta);
    void Update(double delta);
    void RenderFrame();
    void RenderScene(const Camera &cam);
    void RenderScene(const glm::mat4 &view, const glm::mat4 &projection,
                     bool enableSkybox = true);
    void RenderSceneRaw(ShaderProg &shader);
    void RenderText(ShaderProg &s, std::string text, float x, float y,
                    float scale, glm::vec3 colour);

    void HandleEvent(SDL_Event *event);
    void DeleteAllLights();
    /* Returns current aspect ratio of the window */
    float ScreenAspect();
    SDL_Window* GetWindow();
    SDL_GLContext& GetGLContext();
    void CleanUp();

    SunLight& GetSunLight();

}



