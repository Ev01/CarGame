#pragma once


#include <glm/glm.hpp>
#include <assimp/matrix4x4.h>
#include <SDL3/SDL.h>

#include <string>

// forward declarations
struct ShaderProg;
struct Camera;
struct aiLight;
struct aiNode;
struct Player;
//struct aiMatrix4x4;
namespace Render {
    struct SunLight;
}
namespace Font {
    struct Face;
}

namespace Render {
    bool Init();
    void AssimpAddLight(const aiLight *light, const aiNode *node, aiMatrix4x4 transform);
    void PhysicsUpdate(double delta);
    void Update(double delta);
    void RenderFrame();
    void RenderScene(const Camera &cam, Player *p = nullptr);
    void RenderScene(const glm::mat4 &view, const glm::mat4 &projection,
                     bool enableSkybox = true, Player *p = nullptr);
    void RenderSceneRaw(ShaderProg &shader, Player *p = nullptr);

    void HandleEvent(SDL_Event *event);
    void DeleteAllLights();
    /* Returns current aspect ratio of the window */
    float ScreenAspect();
    void UpdatePlayerCamAspectRatios();
    SDL_Window* GetWindow();
    SDL_GLContext& GetGLContext();
    void CleanUp();

    SunLight& GetSunLight();

    void SetDoRenderWorld(bool value);
    bool GetDoRenderWorld();

}



