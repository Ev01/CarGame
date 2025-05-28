#include "render.h"

#include "convert.h"
#include "camera.h"
#include "model.h"
#include "shader.h"
#include "glad/glad.h"
#include "physics.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SDL3/SDL.h>

static std::vector<Render::Light> lights;
static Render::SunLight sunLight;
static ShaderProg shader;
static SDL_Window *window;
static SDL_GLContext context;
static glm::mat4 projection; 

//static const glm::vec3 sunDir = glm::vec3(0.1, -0.5, 0.1);
static const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

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

        default:
            SDL_Log("Light not supported");
            break;
    }
            

}


bool Render::Init()
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(
            SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

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

    // Create shaders
    unsigned int vShader = CreateShaderFromFile("shaders/vertex.glsl", 
                                                 GL_VERTEX_SHADER);
    unsigned int fShader = CreateShaderFromFile("shaders/fragment.glsl", 
                                                 GL_FRAGMENT_SHADER);
    shader = CreateAndLinkShaderProgram(vShader, fShader);

    projection = glm::perspective(SDL_PI_F / 4.0f,
                                  800.0f / 600.0f,
                                  0.1f, 100.0f);

    return true;
}


void Render::RenderFrame(const Camera &cam, const Model &mapModel,
                         const Model &carModel, const Model &wheelModel)
{
    glClearColor(0.7f, 0.6f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 model = glm::mat4(1.0f);
    /*
    model = glm::translate(model, spherePosGlm);
    model = model * QuatToMatrix(Phys::GetSphereRotation());
    model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
    */

    glm::mat4 view = cam.LookAtMatrix(up);
    int windowWidth;
    int windowHeight;
    if (!SDL_GetWindowSize(window, &windowWidth, &windowHeight)) {
        SDL_Log("Could not get window size, using defaults");
        //windowWidth = 800;
        //windowHeight = 600;
    } 

    glUseProgram(shader.id);

    //shader.SetMat4fv((char*)"model", glm::value_ptr(model));
    shader.SetMat4fv((char*)"projection", glm::value_ptr(projection));
    shader.SetMat4fv((char*)"view", glm::value_ptr(view));

    //glm::vec3 lightDir = glm::vec3(view * glm::vec4(sunDir, 0.0f));
    glm::vec3 sunCol = sunLight.mColour / glm::vec3(1000.0);
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

        glm::vec3 lightCol = lights[i].mColour / glm::vec3(1000.0);
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


    shader.SetFloat((char*)"material.shininess", 32.0f);

    /*
    monkeyModel.Draw(shader, model);

    for (size_t i=0; i < NUM_SPHERES; i++){
        model = glm::mat4(1.0f);
        model = glm::translate(model, ToGlmVec3(Phys::GetSpherePos(i)));
        model = model * QuatToMatrix(Phys::GetSphereRotation(i));
        model = glm::scale(model, glm::vec3(0.5f));
        //shader.SetMat4fv((char*)"model", glm::value_ptr(model));
        monkeyModel.Draw(shader, model);
    }
    */

    //shader.SetMat4fv((char*)"model", glm::value_ptr(floorMat));
    //cubeModel.Draw(shader, floorMat);

    // Draw map
    model = glm::mat4(1.0f);
    shader.SetMat4fv((char*)"model", glm::value_ptr(model));
    mapModel.Draw(shader);

    // Draw Car
    glm::vec3 carPos = ToGlmVec3(Phys::GetCarPos());
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
}

void Render::HandleEvent(SDL_Event *event)
{
    if (event->type == SDL_EVENT_WINDOW_RESIZED) {
        int width = event->window.data1;
        int height = event->window.data2;
        glViewport(0, 0, width, height);
        projection = glm::perspective(SDL_PI_F / 4.0f,
                                      (float) width / height,
                                      0.1f, 100.0f);
    }
    else if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_F11) {
        SDL_SetWindowFullscreen(window, !(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN));
    }
}



SDL_Window* Render::Window()    { return window; }

