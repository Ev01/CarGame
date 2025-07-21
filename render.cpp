#include "render.h"

#include "convert.h"
#include "camera.h"
#include "input.h"
#include "model.h"
#include "shader.h"
#include "glad/glad.h"
#include "physics.h"
#include "vehicle.h"
#include "world.h"

#include "vendor/imgui/imgui.h"
#include "vendor/imgui/backends/imgui_impl_opengl3.h"
#include "vendor/imgui/backends/imgui_impl_sdl3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <SDL3/SDL.h>

static std::vector<Render::Light> lights;
static std::vector<Render::SpotLight*> spotLights;
static Render::SunLight sunLight;
static ShaderProg shader;
static ShaderProg skyboxShader;
static ShaderProg screenShader;
static SDL_Window *window;
static SDL_GLContext context;

static Texture skyboxTex;

static Model *cubeModel;
//static Camera cam;
static VehicleCamera cam2;
static VehicleCamera cam3;
static int currentCamNum = 0;
// Camera Settings
static float camPitch = -0.4f;
static float camDist = 3.7f;
static float angleSmooth = 3.5f;
static float distSmooth = 17.0f;
static bool doSplitScreen = false;

static float skyboxVertices[] = {
    // positions          
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};

static float quadVertices[] = {
    // Positions  // Tex Coords
    -1.0f, 1.0f,  0.0f, 1.0f,     // Top left
    1.0f, 1.0f,    1.0f, 1.0f,     // Top right
    1.0f, -1.0f,   1.0f, 0.0f,    // Bottom right
    1.0f, -1.0f,   1.0f, 0.0f,    // Bottom right
    -1.0f, -1.0f, 0.0f, 0.0f,    // Bottom left
    -1.0f, 1.0f,  0.0f, 1.0f      // Top left
};


static unsigned int skyboxVAO;
static unsigned int skyboxVBO;

static unsigned int fbo;
static unsigned int rbo;
static unsigned int textureColourBuffer;
static unsigned int msFBO;
static unsigned int msRBO;
static unsigned int msTexColourBuffer;


static unsigned int quadVAO;
static unsigned int quadVBO;

static const int fbWidth = 1600;
static const int fbHeight = 900;
//static const int fbWidth = 800;
//static const int fbHeight = 600;


//static const glm::vec3 sunDir = glm::vec3(0.1, -0.5, 0.1);
static const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);


static void CreateFramebuffer(unsigned int *aFBO, unsigned int *aCbTex, unsigned int *aRBO)
{
    glGenFramebuffers(1, aFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, *aFBO);

    // Create texture for frame buffer
    glGenTextures(1, aCbTex);
    glBindTexture(GL_TEXTURE_2D, *aCbTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, fbWidth, fbHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    // Attach texture to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *aCbTex, 0);
    // Create renderbuffer for depth and stencil components
    glGenRenderbuffers(1, aRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, *aRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, fbWidth, fbHeight);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    // Attach render buffer to framebuffer
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, *aRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        SDL_Log("Error: Framebuffer is not complete!");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


static void CreateMSFramebuffer(unsigned int *aFBO, unsigned int *aCbTex, unsigned int *aRBO)
{
    glGenFramebuffers(1, aFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, *aFBO);

    // Create texture for frame buffer
    glGenTextures(1, aCbTex);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, *aCbTex);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB32F, fbWidth, fbHeight, GL_TRUE);
    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    // Attach texture to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, *aCbTex, 0);
    // Create renderbuffer for depth and stencil components
    glGenRenderbuffers(1, aRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, *aRBO);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, fbWidth, fbHeight);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    // Attach render buffer to framebuffer
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, *aRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        SDL_Log("Error: Framebuffer is not complete!");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void Render::AssimpAddLight(const aiLight *aLight, const aiNode *aNode, aiMatrix4x4 aTransform)
{
    aiQuaternion rotation;
    aiVector3D position;
    glm::mat4 glmTransform = ToGlmMat4(aTransform);
    switch (aLight->mType) {
        case aiLightSource_UNDEFINED:
            SDL_Log("Undefined light source!");
            break;

        case aiLightSource_POINT:
            SDL_Log("Adding Point light %s...", aLight->mName.C_Str());
            Light newLight;

            aTransform.DecomposeNoScaling(rotation, position);

            newLight.mPosition = ToGlmVec3(position);
            newLight.mColour = ToGlmVec3(aLight->mColorDiffuse);
            SDL_Log("Colour = (%f, %f, %f)", newLight.mColour.x, newLight.mColour.y, newLight.mColour.z);
            //newLight.mDirection = toGlmVec3(aLight->mDirection);
            // NOTE: always 1 for gltf imports. Intensity would be stored in colour
            newLight.mQuadratic = aLight->mAttenuationQuadratic;

            lights.push_back(newLight);
            break;

        case aiLightSource_DIRECTIONAL:
            SDL_Log("Adding Sun Light...");
            //aTransform.DecomposeNoScaling(rotation, position);
            sunLight.mDirection = glm::vec3(glmTransform * glm::vec4(0.0, 0.0, -1.0, 0.0));
            sunLight.mColour = ToGlmVec3(aLight->mColorDiffuse);
            SDL_Log("Sun direction = %f, %f, %f", sunLight.mDirection.x, sunLight.mDirection.y, sunLight.mDirection.z);
            break;

        /*
        case aiLightSource_SPOT:
            SDL_Log("Adding Spot light %s...", aNode->mName.C_Str());
            SpotLight spotLight;

            aTransform.DecomposeNoScaling(rotation, position);
            glm::vec3 position2;
            position2 = glm::vec3(glmTransform * glm::vec4(ToGlmVec3(aLight->mPosition), 1.0));

            //SDL_Log("light pos (%f, %f, %f)", aLight->mPosition.x, aLight->mPosition.y, aLight->mPosition.z);

            //spotLight.mPosition = ToGlmVec3(position);
            spotLight.mPosition = position2;
            glm::vec3 dir;
            dir = ToGlmVec3(aLight->mDirection);
            spotLight.mDirection = glm::vec3(glmTransform * glm::vec4(dir, 0.0));
            spotLight.mColour = ToGlmVec3(aLight->mColorDiffuse);
            spotLight.mQuadratic = aLight->mAttenuationQuadratic;
            spotLight.mCutoffInner = SDL_cos(aLight->mAngleInnerCone);
            spotLight.mCutoffOuter = SDL_cos(aLight->mAngleOuterCone);

            spotLights.push_back(spotLight);
            break;
        */

        default:
            //SDL_Log("Light not supported");
            break;
    }
            

}


Render::SpotLight* Render::CreateSpotLight()
{
    SpotLight *newSpotLight = new SpotLight;
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



/*
Render::SpotLight& Render::GetSpotLightById(unsigned int id)
{
    SDL_assert(spotLights.size() > id); // This spotlight was deleted
    return spotLights[id];
}
*/
    


bool Render::Init()
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(
            SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    window = SDL_CreateWindow("Car", 800, 600, flags);

    if (window == NULL) {
        SDL_Log("Could not create window: %s", SDL_GetError());
        return false;
    }

    context = SDL_GL_CreateContext(window);

    // Load opengl functions
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        SDL_Log("Failed to initialise GLAD");
        return false;
    }

    if (!SDL_GL_SetSwapInterval(1)) {
        SDL_Log("Could not turn on VSync");
    }

    // Create shaders
    unsigned int vShader = CreateShaderFromFile("shaders/vertex.glsl", 
                                                 GL_VERTEX_SHADER);
    unsigned int fShader = CreateShaderFromFile("shaders/fragment.glsl", 
                                                 GL_FRAGMENT_SHADER);
    shader = CreateAndLinkShaderProgram(vShader, fShader);

    unsigned int vSkybox = CreateShaderFromFile("shaders/v_skybox.glsl",
                                                GL_VERTEX_SHADER);
    unsigned int fSkybox = CreateShaderFromFile("shaders/f_skybox.glsl",
                                                GL_FRAGMENT_SHADER);
    skyboxShader = CreateAndLinkShaderProgram(vSkybox, fSkybox);

    unsigned int vScreen = CreateShaderFromFile("shaders/v_screen.glsl",
                                                GL_VERTEX_SHADER);
    unsigned int fScreen = CreateShaderFromFile("shaders/f_screen.glsl",
                                                GL_FRAGMENT_SHADER);
    screenShader = CreateAndLinkShaderProgram(vScreen, fScreen);

    // Skybox VAO
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    //glDisableVertexAttribArray(1);
    //glDisableVertexAttribArray(2);
    skyboxTex = CreateCubemapFromFiles("texture/Lycksele/posx.jpg",
                                       "texture/Lycksele/negx.jpg",
                                       "texture/Lycksele/posy.jpg",
                                       "texture/Lycksele/negy.jpg",
                                       "texture/Lycksele/posz.jpg",
                                       "texture/Lycksele/negz.jpg");
    // ----- Quad VAO -----
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    // Cube
    cubeModel = LoadModel("models/cube.gltf");

    // Camera
    /*
    cam.Init(SDL_PI_F / 4.0, 800.0f / 600.0f, 0.1f, 1000.0f);
    */
    const float fov = glm::radians(95.0);
    //const float pitch = 
    float aspect = ScreenAspect();
    cam2.Init(fov, aspect, 0.1f, 1000.0f);
    cam3.Init(fov, aspect, 0.1f, 1000.0f);
    
    //cam2.cam.pos.z = 6.0f;
    //cam2.cam.SetYawPitch(-SDL_PI_F / 2.0, 0);

    // ----- Framebuffer -----
    CreateFramebuffer(&fbo, &textureColourBuffer, &rbo);
    CreateMSFramebuffer(&msFBO, &msTexColourBuffer, &msRBO);

    // Enable MSAA (anti-aliasing)
    glEnable(GL_MULTISAMPLE);

    return true;
}


float Render::ScreenAspect()
{
    int screenWidth, screenHeight;
    bool screenSuccess = SDL_GetWindowSize(window, &screenWidth, &screenHeight);
    if (screenSuccess) {
        // Divide aspect by 2.0 for split screen
        float aspect = (float) screenWidth / (float) screenHeight;
        if (doSplitScreen) {
            aspect /= 2.0;
        }
        return aspect;
    }
    return 0.0;
}


Camera& Render::GetCamera()
{
    return cam2.cam;
}


void Render::PhysicsUpdate(double delta)
{
    float yawOffset = 0;
    if (Input::GetGamepad()) {
        yawOffset = SDL_PI_F / 2.0f * Input::GetGamepadAxis(SDL_GAMEPAD_AXIS_RIGHTX);
    }
    float yawOffset2 = SDL_PI_F / 2.0f * Input::GetScanAxis(SDL_SCANCODE_D, SDL_SCANCODE_A);
    cam2.targetBody = World::GetCar().mBody;
    cam3.targetBody = World::GetCar2().mBody;
    //SDL_Log("Car yaw: %f, x: %f, z: %f", carYaw, carDir.GetX(), carDir.GetZ());
    //cam.SetFollowSmooth(carYaw + yawOffset, camPitch, camDist, carPos, 
    //                    angleSmooth * delta, distSmooth * delta);
    cam2.SetFollowSmooth(yawOffset2, camPitch, camDist, 
                        angleSmooth * delta, distSmooth * delta);
    cam3.SetFollowSmooth(yawOffset, camPitch, camDist, 
                        angleSmooth * delta, distSmooth * delta);
}


void Render::Update(double delta)
{
    // Update Camera
    ImGui::Begin("Cool window");
    float fovDegrees = glm::degrees(cam2.cam.fov);
    ImGui::SliderFloat("FOV", &fovDegrees, 20.0f, 140.0f);
    cam2.cam.SetFovAndRecalcProjection(glm::radians(fovDegrees));
    cam3.cam.SetFovAndRecalcProjection(glm::radians(fovDegrees));
    ImGui::SliderFloat("Camera Pitch", &camPitch,
                       -SDL_PI_F / 2.0f, SDL_PI_F / 2.0);
    ImGui::SliderFloat("Camera Distance", &camDist, 0.0f, 30.0f);
    ImGui::SliderFloat("Camera Angle Smoothing", &angleSmooth, 0.0f, 20.0f);
    ImGui::SliderFloat("Camera Distance Smoothing", &distSmooth, 0.0f, 20.0f);

    const char* items[] = {"Cam1", "Cam2"};
    ImGui::Combo("Camera", &currentCamNum, items, 2);
    bool splitBefore = doSplitScreen;
    ImGui::Checkbox("Splitscreen", &doSplitScreen);
    if (doSplitScreen != splitBefore) {
        float aspect = ScreenAspect();
        cam2.cam.aspect = aspect;
        cam3.cam.aspect = aspect;
        cam2.cam.CalcProjection();
        cam3.cam.CalcProjection();
    }
    ImGui::End();
}


void Render::RenderFrame()
{
    int screenWidth, screenHeight;
    bool screenSuccess = SDL_GetWindowSize(window, &screenWidth, &screenHeight);
    if (screenSuccess) {
        glViewport(0, 0, screenWidth, screenHeight);
    }


    glBindFramebuffer(GL_FRAMEBUFFER, msFBO);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.7f, 0.6f, 0.2f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render left screen
    int playerScreenWidth = doSplitScreen ? screenWidth / 2 : screenWidth;
    if (screenSuccess) {
        glViewport(0, 0, playerScreenWidth, screenHeight);
    }
    RenderScene(cam2.cam);
    
    // Render right screen
    if (doSplitScreen) {
        if (screenSuccess) {
            glViewport(screenWidth/2, 0, playerScreenWidth, screenHeight);
        }
        RenderScene(cam3.cam);
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, msFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
    glBlitFramebuffer(0, 0, screenWidth, screenHeight,
                      0, 0, screenWidth, screenHeight,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);


    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0, 0, fbWidth, fbHeight);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);
    glUseProgram(screenShader.id);
    glBindVertexArray(quadVAO);
    glBindTexture(GL_TEXTURE_2D, textureColourBuffer);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindTexture(GL_TEXTURE_2D, 0);



    //static bool showDemoWindow = true;
    //ImGui::ShowDemoWindow(&showDemoWindow);

    

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    SDL_GL_SwapWindow(window);
}


void Render::RenderScene(const Camera &cam)
{
    glm::mat4 view;
    glm::mat4 projection;
    view = cam.LookAtMatrix(up);
    projection = cam.projection;

    glm::mat4 model = glm::mat4(1.0f);
    int windowWidth;
    int windowHeight;
    if (!SDL_GetWindowSize(window, &windowWidth, &windowHeight)) {
        SDL_Log("Could not get window size, using defaults");
        windowWidth = 800;
        windowHeight = 600;
    } 

    glUseProgram(shader.id);

    shader.SetMat4fv((char*)"projection", glm::value_ptr(projection));
    shader.SetMat4fv((char*)"view", glm::value_ptr(view));

    glm::vec3 sunCol = sunLight.mColour / glm::vec3(1.0);
    glm::vec3 sunDir = glm::vec3(view * glm::vec4(sunLight.mDirection, 0.0));
    shader.SetVec3((char*)"dirLight.direction", glm::value_ptr(sunDir));
    shader.SetVec3((char*)"dirLight.ambient", 0.1, 0.1, 0.1);
    shader.SetVec3((char*)"dirLight.diffuse", glm::value_ptr(sunCol));
    shader.SetVec3((char*)"dirLight.specular", glm::value_ptr(sunCol));

    char uniformName[64];
    for (size_t i = 0; i < lights.size(); i++) {
        SDL_snprintf(uniformName, 64, "pointLights[%llu].position", i);
        glm::vec3 viewPos = glm::vec3(view * glm::vec4(lights[i].mPosition, 1.0));
        shader.SetVec3(uniformName, glm::value_ptr(viewPos));

        glm::vec3 lightCol = lights[i].mColour / glm::vec3(1.0);
        //glm::vec3 lightCol = glm::vec3(6000.0);
        SDL_snprintf(uniformName, 64, "pointLights[%llu].ambient", i);
        shader.SetVec3(uniformName, 0.0, 0.0, 0.0);
        SDL_snprintf(uniformName, 64, "pointLights[%llu].diffuse", i);
        shader.SetVec3(uniformName, glm::value_ptr(lightCol));
        SDL_snprintf(uniformName, 64, "pointLights[%llu].specular", i);
        shader.SetVec3(uniformName, glm::value_ptr(lightCol));

        //SDL_Log("Colour = (%f, %f, %f)", lights[i].mColour.x, lights[i].mColour.y, lights[i].mColour.z);

        SDL_snprintf(uniformName, 64, "pointLights[%llu].quadratic", i);
        shader.SetFloat(uniformName, 1.0f);
        SDL_snprintf(uniformName, 64, "pointLights[%llu].constant", i);
        shader.SetFloat(uniformName, 0.0f);
        SDL_snprintf(uniformName, 64, "pointLights[%llu].linear", i);
        shader.SetFloat(uniformName, 0.0f);
    }

    for (size_t i = 0; i < spotLights.size(); i++) {
        if (spotLights[i] == nullptr) continue;
        glm::vec3 lightCol = spotLights[i]->mColour / glm::vec3(1.0);
        //glm::vec3 lightCol = glm::vec3(6000.0);
        SDL_snprintf(uniformName, 64, "spotLights[%llu].diffuse", i);
        shader.SetVec3(uniformName, glm::value_ptr(lightCol));
        SDL_snprintf(uniformName, 64, "spotLights[%llu].specular", i);
        shader.SetVec3(uniformName, glm::value_ptr(lightCol));

        glm::vec3 viewPos = glm::vec3(view * glm::vec4(spotLights[i]->mPosition, 1.0));
        SDL_snprintf(uniformName, 64, "spotLights[%llu].position", i);
        shader.SetVec3(uniformName, glm::value_ptr(viewPos));

        glm::vec3 viewDir = glm::vec3(view * glm::vec4(spotLights[i]->mDirection, 0.0));
        SDL_snprintf(uniformName, 64, "spotLights[%llu].direction", i);
        shader.SetVec3(uniformName, glm::value_ptr(viewDir));

        SDL_snprintf(uniformName, 64, "spotLights[%llu].quadratic", i);
        shader.SetFloat(uniformName, spotLights[i]->mQuadratic);
        SDL_snprintf(uniformName, 64, "spotLights[%llu].cutoffInner", i);
        shader.SetFloat(uniformName, spotLights[i]->mCutoffInner);
        SDL_snprintf(uniformName, 64, "spotLights[%llu].cutoffOuter", i);
        shader.SetFloat(uniformName, spotLights[i]->mCutoffOuter);
    }


    shader.SetFloat((char*)"material.shininess", 32.0f);

    // Draw map
    Model &mapModel = World::GetCurrentMapModel();
    model = glm::mat4(1.0f);
    shader.SetMat4fv((char*)"model", glm::value_ptr(model));
    mapModel.Draw(shader);

    // Draw all cars
    for (Vehicle *car : GetExistingVehicles()) {
        glm::vec3 carPos = ToGlmVec3(car->GetPos());
        glm::mat4 carTrans = glm::mat4(1.0f);
        carTrans = glm::translate(carTrans, carPos);
        carTrans = carTrans * QuatToMatrix(car->GetRotation());

        car->mVehicleModel->Draw(shader, carTrans);

        // Draw car wheels
        for (int i = 0; i < 4; i++) {
            glm::mat4 wheelTrans = ToGlmMat4(car->GetWheelTransform(i));
            if (car->IsWheelFlipped(i)) {
                wheelTrans = glm::rotate(wheelTrans, SDL_PI_F, glm::vec3(1.0f, 0.0f, 0.0f));
            }
            car->mWheelModel->Draw(shader, wheelTrans);
        }
    }

    // Draw checkpoints
    //for (const Checkpoint &checkpoint : World::GetCheckpoints()) {
    //    if (checkpoint.mIsCollected) continue;
    if (World::GetCheckpoints().size() > 0) {
        Checkpoint &checkpoint = World::GetCheckpoints()[gPlayerRaceProgress.mCheckpointsCollected];
        model = glm::mat4(1.0f);
        model = glm::translate(model, ToGlmVec3(checkpoint.GetPosition()));
        model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f));
        cubeModel->Draw(shader, model);
    }
    //    break; // Only Draw checkpoint needed next
    //}


    // Draw skybox
    
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    // Remove translation component from view matrix
    glm::mat4 skyboxView = glm::mat4(glm::mat3(view));
    glUseProgram(skyboxShader.id);

    skyboxShader.SetMat4fv((char*)"projection", glm::value_ptr(projection));
    skyboxShader.SetMat4fv((char*)"view", glm::value_ptr(skyboxView));
    skyboxShader.SetInt((char*)"skybox", 0);

    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex.id);
    glBindVertexArray(skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
}


void Render::HandleEvent(SDL_Event *event)
{
    if (event->type == SDL_EVENT_WINDOW_RESIZED) {
        int width = event->window.data1;
        int height = event->window.data2;
        glViewport(0, 0, width, height);

        // Divide aspect by 2.0 for split screen
        float aspect = (float) width / height;
        if (doSplitScreen) {
            aspect /= 2;
        }
        cam2.cam.aspect = aspect;
        cam2.cam.CalcProjection();
        cam3.cam.aspect = aspect;
        cam3.cam.CalcProjection();
    }
    else if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_F11) {
        SDL_SetWindowFullscreen(window, !(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN));
    }
}


void Render::DeleteAllLights()
{
    lights.clear();
    //spotLights.clear();
    sunLight.mDirection = glm::vec3(0.0);
    sunLight.mColour = glm::vec3(0.0);
}


void Render::CleanUp()
{
    delete cubeModel;
    glDeleteFramebuffers(1, &fbo);
}


SDL_Window* Render::GetWindow()            { return window; }
SDL_GLContext& Render::GetGLContext()      { return context; }


