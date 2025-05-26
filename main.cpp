#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

#include "physics.h"
#include "convert.h"

#include "glad/glad.h"
#include "model.h"
#include "shader.h"
#include "camera.h"
#include "input.h"
#include "audio.h"

#define MOUSE_SENSITIVITY 0.001f
#define CAM_SPEED 4.0f
#define PHYSICS_STEP_TIME 1.0 / 60


SDL_Window *window;
SDL_GLContext context;

ShaderProg shader;
Model monkeyModel;
Model cubeModel;
Model cylinderModel;
Model carModel;
Model wheelModel;
Model mapModel;
Camera cam;

const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

double delta, lastFrame;
float physicsTime = 0;
glm::vec3 spherePosGlm;
glm::vec3 sphere2PosGlm;
glm::vec3 sunDir = glm::vec3(0.1, -0.5, 0.1);

float yaw = -SDL_PI_F / 2.0;
float pitch;


double getSeconds()
{
    return (double) SDL_GetTicksNS() / (double) SDL_NS_PER_SECOND;
}

bool carNodeCallback(aiNode *node, aiMatrix4x4 transform)
{
    aiQuaternion aRotation;
    aiVector3D aPosition;
    aiVector3D aScale;
    transform.Decompose(aScale, aRotation, aPosition);

    JPH::RMat44 joltTransform = ToJoltMat4(transform);
    JPH::Vec3 position = joltTransform.GetTranslation();
    if (SDL_strcmp(node->mName.C_Str(), "WheelPosFR") == 0) {
        //SDL_Log("FR, %s", node->mName.C_Str());

        Phys::AddCarWheel(position, true);
    }
    else if (SDL_strcmp(node->mName.C_Str(), "WheelPosFL") == 0) {
        //SDL_Log("FL");
        Phys::AddCarWheel(position, true);
    }
    else if (SDL_strcmp(node->mName.C_Str(), "WheelPosRR") == 0) {
        //SDL_Log("RR");
        Phys::AddCarWheel(position, false);
    }
    else if (SDL_strcmp(node->mName.C_Str(), "WheelPosRL") == 0) {
        Phys::AddCarWheel(position, false);
    }
    else if (SDL_strncmp(node->mName.C_Str(), "CollisionBox", 12) == 0) {
        Phys::AddCarCollisionBox(ToJoltVec3(aPosition), ToJoltVec3(aScale));
    }
    return true;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(
            SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    int flags = SDL_WINDOW_OPENGL;
    window = SDL_CreateWindow("Car", 800, 600, flags);

    if (window == NULL) {
        SDL_Log("Could not create window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    context = SDL_GL_CreateContext(window);

    // Load opengl functions
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        SDL_Log("Failed to initialise GLAD");
        return SDL_APP_FAILURE;
    }

    InitDefaultTexture();

    // Create shaders
    unsigned int vShader = CreateShaderFromFile("shaders/vertex.glsl", 
                                                 GL_VERTEX_SHADER);
    unsigned int fShader = CreateShaderFromFile("shaders/fragment.glsl", 
                                                 GL_FRAGMENT_SHADER);
    shader = CreateAndLinkShaderProgram(vShader, fShader);

    // Audio
    Audio::Init();

    Phys::SetupJolt();


    // Load model
    monkeyModel = LoadModel("models/monkey.obj");
    cubeModel = LoadModel("models/cube.obj");
    cylinderModel = LoadModel("models/cylinder.obj");
    carModel = LoadModel("models/mycar.gltf", carNodeCallback);
    wheelModel = LoadModel("models/wheel.gltf");
    mapModel = LoadModel("models/simple_map.gltf");

    cam.pos.z = 6.0f;
    cam.SetYawPitch(yaw, pitch);

    Phys::SetupSimulation();
    Phys::LoadMap(mapModel);

    glViewport(0, 0, 800, 600);

    SDL_SetWindowRelativeMouseMode(window, true);
    /*
    if (!SDL_SetWindowSurfaceVSync(window, 1)) {
        SDL_Log("Couldn't set VSync: %s", SDL_GetError());
    }
    */
    glEnable(GL_DEPTH_TEST);

    lastFrame = getSeconds();

    return SDL_APP_CONTINUE;
}


SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    Input::HandleEvent(event);

    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    else if (event->type == SDL_EVENT_MOUSE_MOTION) {
        yaw += event->motion.xrel * MOUSE_SENSITIVITY;
        pitch -= event->motion.yrel * MOUSE_SENSITIVITY;
        pitch = SDL_clamp(pitch, -SDL_PI_F / 2.0 + 0.1, SDL_PI_F / 2.0 - 0.1);
    }
    else if (event->type == SDL_EVENT_KEY_DOWN && event->key.scancode == SDL_SCANCODE_F) {
        SDL_Log("FPS: %f", 1.0/delta);
    }

    return SDL_APP_CONTINUE;
}


SDL_AppResult SDL_AppIterate(void *appstate)
{
    delta = getSeconds() - lastFrame;
    lastFrame = getSeconds();


    // Do physics step
    physicsTime += delta;
    while (physicsTime >= PHYSICS_STEP_TIME) {
        Phys::SetForwardDir(ToJoltVec3(cam.dir));
        Phys::ProcessInput();
        Phys::PhysicsStep(PHYSICS_STEP_TIME);
        physicsTime -= PHYSICS_STEP_TIME;

        JPH::RVec3 spherePos = Phys::GetSpherePos();
        spherePosGlm = glm::vec3(spherePos.GetX(), spherePos.GetY(), spherePos.GetZ());
        /*
        JPH::RVec3 sphere2Pos = Phys::getSphere2Pos();
        sphere2PosGlm = glm::vec3(sphere2Pos.GetX(), sphere2Pos.GetY(), sphere2Pos.GetZ());
        */
        //SDL_Log("sphere pos: %f, %f, %f", spherePosGlm.x, spherePosGlm.y, spherePosGlm.z);
    }

    // Audio
    Audio::Update();

    glm::vec3 carPos = ToGlmVec3(Phys::GetCarPos());
    JPH::Quat carRot = Phys::GetCarRotation();
    JPH::Vec3 carDir = carRot.RotateAxisX();
    float carYaw = SDL_PI_F - SDL_atan2f(carDir.GetX(), carDir.GetZ());
    float yawOffset;
    if (Input::GetGamepad()) {
        yawOffset = SDL_PI_F / 2.0f * Input::GetGamepadAxis(SDL_GAMEPAD_AXIS_RIGHTX);
    }
    //SDL_Log("Car yaw: %f, x: %f, z: %f", carYaw, carDir.GetX(), carDir.GetZ());
    cam.SetFollowSmooth(carYaw + yawOffset, -0.2f, 14.0f, carPos, 3.0f * delta);
    //cam.SetOrbit(14.0f, 4.0f, carPos);
    // Move Camera
    /*
    if (Input::IsScanDown(SDL_SCANCODE_A)) {
        cam.pos -= cam.Right(up) * CAM_SPEED * (float)delta;
    }
    if (Input::IsScanDown(SDL_SCANCODE_D)) {
        cam.pos += cam.Right(up) * CAM_SPEED * (float)delta;
    }
    if (Input::IsScanDown(SDL_SCANCODE_W)) {
        cam.pos += cam.dir * CAM_SPEED * (float)delta;
    }
    if (Input::IsScanDown(SDL_SCANCODE_S)) {
        cam.pos -= cam.dir * CAM_SPEED * (float)delta;
    }
    */
    

    glClearColor(0.7f, 0.6f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /*
    glm::mat4 floorMat = glm::mat4(1.0f);
    floorMat = glm::scale(floorMat, glm::vec3(100.0f, 1.0f, 100.0f));
    floorMat = glm::translate(floorMat, glm::vec3(0.0f, -2.0f, 0.0f));
    */

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, spherePosGlm);
    model = model * QuatToMatrix(Phys::GetSphereRotation());
    model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));

    /*
    glm::mat4 model2 = glm::mat4(1.0f);
    model2 = glm::translate(model2, sphere2PosGlm);
    model2 = model2 * QuatToMatrix(Phys::getSphere2Rotation());
    model2 = glm::scale(model2, glm::vec3(0.25f, 0.25f, 0.25f));
    */
    
    glm::mat4 view = cam.LookAtMatrix(up);
    static glm::mat4 projection = glm::perspective(SDL_PI_F / 4.0f,
                                            800.0f / 600.0f,
                                            0.1f, 100.0f);

    glUseProgram(shader.id);

    //shader.SetMat4fv((char*)"model", glm::value_ptr(model));
    shader.SetMat4fv((char*)"projection", glm::value_ptr(projection));
    shader.SetMat4fv((char*)"view", glm::value_ptr(view));

    glm::vec3 lightDir = glm::vec3(view * glm::vec4(sunDir, 0.0f));
    shader.SetVec3((char*)"dirLight.direction", glm::value_ptr(lightDir));
    shader.SetVec3((char*)"dirLight.ambient", 0.2f, 0.2f, 0.2f);
    shader.SetVec3((char*)"dirLight.diffuse", 0.8f, 0.8f, 0.8f);
    shader.SetVec3((char*)"dirLight.specular", 1.0f, 1.0f, 1.0f);

    shader.SetFloat((char*)"material.shininess", 32.0f);

    monkeyModel.Draw(shader, model);

    for (size_t i=0; i < NUM_SPHERES; i++){
        model = glm::mat4(1.0f);
        model = glm::translate(model, ToGlmVec3(Phys::GetSpherePos(i)));
        model = model * QuatToMatrix(Phys::GetSphereRotation(i));
        model = glm::scale(model, glm::vec3(0.5f));
        //shader.SetMat4fv((char*)"model", glm::value_ptr(model));
        monkeyModel.Draw(shader, model);
    }

    //shader.SetMat4fv((char*)"model", glm::value_ptr(floorMat));
    //cubeModel.Draw(shader, floorMat);

    // Draw map
    model = glm::mat4(1.0f);
    shader.SetMat4fv((char*)"model", glm::value_ptr(model));
    mapModel.Draw(shader);

    // Draw Car
    glm::mat4 carTrans = glm::mat4(1.0f);
    carTrans = glm::translate(carTrans, carPos);
    carTrans = carTrans * QuatToMatrix(Phys::GetCarRotation());
    //carTrans = glm::scale(carTrans, glm::vec3(0.9f, 0.2f, 2.0f));

    //shader.SetMat4fv((char*)"model", glm::value_ptr(carTrans));
    carModel.Draw(shader, carTrans);

    // Draw car wheels
    for (int i = 0; i < 4; i++) {
        glm::mat4 wheelTrans = ToGlmMat4(Phys::GetWheelTransform(i));
        if (Phys::IsWheelFlipped(i)) {
            wheelTrans = glm::rotate(wheelTrans, SDL_PI_F, glm::vec3(1.0f, 0.0f, 0.0f));
        }
        //wheelTrans = glm::scale(wheelTrans, glm::vec3(0.45f, 0.2f, 0.45f));
        //shader.SetMat4fv((char*)"model", glm::value_ptr(wheelTrans));
        //cylinderModel.Draw(shader);
        wheelModel.Draw(shader, wheelTrans);
    }



    
    SDL_GL_SwapWindow(window);

    double delta2 = getSeconds() - lastFrame;

    constexpr double sfp = 1.0 / 300.0;
    //SDL_Log("delta: %f", delta);
    if (delta2 < sfp) {
        double toDelaySeconds = (sfp - delta2);
        //SDL_Log("Delta is %f. Delaying %f nanoseconds. SFP: %f", delta2, toDelaySeconds, sfp);
        //double delayBefore = getSeconds();
        SDL_DelayPrecise((Uint64)(toDelaySeconds * 1000000000.0));
        //SDL_Log("Real delay: %f", getSeconds() - delayBefore);
    }
    return SDL_APP_CONTINUE;
}


void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    Phys::PhysicsCleanup();
}
    

