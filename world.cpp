#include "world.h"
#include "render.h"
#include "convert.h"

#include <assimp/matrix4x4.h>
#include <assimp/types.h>
#include <assimp/vector3.h>

#include <glm/glm.hpp>

#include <vector>

static std::vector<Render::SpotLight*> spotLights;

void World::AssimpAddLight(const aiLight *aLight, const aiNode *aNode,
                           aiMatrix4x4 aTransform)
{
    aiQuaternion rotation;
    aiVector3D position;
    glm::mat4 glmTransform = ToGlmMat4(aTransform);
    switch (aLight->mType) {
        /*
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
        */

        case aiLightSource_SPOT:
        {
            SDL_Log("Adding Spot light %s...", aNode->mName.C_Str());
            Render::SpotLight *spotLight = Render::CreateSpotLight();

            aTransform.DecomposeNoScaling(rotation, position);
            glm::vec3 position2;
            position2 = glm::vec3(glmTransform * glm::vec4(ToGlmVec3(aLight->mPosition), 1.0));

            spotLight->mPosition = position2;
            glm::vec3 dir;
            dir = ToGlmVec3(aLight->mDirection);
            spotLight->mDirection = glm::vec3(glmTransform * glm::vec4(dir, 0.0));
            spotLight->mColour = ToGlmVec3(aLight->mColorDiffuse);
            spotLight->mQuadratic = aLight->mAttenuationQuadratic;
            spotLight->mCutoffInner = SDL_cos(aLight->mAngleInnerCone);
            spotLight->mCutoffOuter = SDL_cos(aLight->mAngleOuterCone);

            SDL_Log("Dir %f, %f, %f", spotLight->mDirection.x, spotLight->mDirection.y, spotLight->mDirection.z);
            SDL_Log("Col %f, %f, %f", spotLight->mColour.x, spotLight->mColour.y, spotLight->mColour.z);
            SDL_Log("Cutoff inner, outer: %f, %f", spotLight->mCutoffInner, spotLight->mCutoffOuter);
            SDL_Log("pos: %f, %f, %f", spotLight->mPosition.x, spotLight->mPosition.y, spotLight->mPosition.z);

            spotLights.push_back(spotLight);
            break;
        }

        default:
            break;
    }
}


void World::DestroyAllLights()
{
    for (Render::SpotLight *spotLight : spotLights) {
        Render::DestroySpotLight(spotLight);
    }
    spotLights.clear();
}



