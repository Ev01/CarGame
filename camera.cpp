#include <glm/glm.hpp>
#include <SDL3/SDL.h>

#include "camera.h"

glm::vec3 Camera::Target() const
{
    return pos + dir;
}

glm::vec3 Camera::Right(glm::vec3 up) const
{
    return -glm::normalize(glm::cross(up, dir));
}

void Camera::SetYawPitch(float yaw, float pitch) {
    dir.y = SDL_sinf(pitch);
    dir.x = SDL_cosf(yaw);// * SDL_cosf(pitch);
    dir.z = SDL_sinf(yaw);// * SDL_cosf(pitch);
    //SDL_Log("yaw: %f, Cam dir x: %f, dir z: %f", yaw, dir.x, dir.z);
}

glm::mat4 Camera::LookAtMatrix(glm::vec3 up) const
{
    glm::mat4 view = glm::lookAt(pos, Target(), up);
    return view;
}


void Camera::SetFollow(float yaw, float pitch, float dist, glm::vec3 targ)
{
    SetYawPitch(yaw, pitch);
    glm::vec3 posOffset = dir * dist;
    pos = targ - posOffset;
}


void Camera::SetFollowSmooth(float yaw, float pitch, float dist, glm::vec3 targ, float smoothing)
{
    glm::vec3 hDir = dir;
    hDir.y = 0;
    hDir = glm::normalize(hDir);

    float currYaw = SDL_PI_F - SDL_atan2f(hDir.z, -hDir.x);

    // Do an angular lerp. 
    // This could be a more simple solution: 
    // https://stackoverflow.com/a/30129248/20829099
    
    float max = SDL_PI_F * 2.0f;
    float da = SDL_fmodf(yaw - currYaw, max);
    float angleDist = SDL_fmodf(2*da, max) - da;
    float newYaw = currYaw + angleDist * smoothing;
    //SDL_Log("currYaw: %f, newYaw: %f, x: %f, z: %f", currYaw, newYaw, hDir.x, hDir.z);

    SetFollow(newYaw, pitch, dist, targ);
}


void Camera::SetOrbit(float dist, float height, glm::vec3 targ)
{
    glm::vec3 difference = targ - pos;
    difference.y = 0;
    difference = glm::normalize(difference);
    pos = targ - difference * dist;
    pos.y = targ.y + height;
    dir = targ - pos;
}
