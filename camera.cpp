#include "camera.h"

#include <glm/glm.hpp>
#include <SDL3/SDL.h>



void Camera::Init(float aFov, float aAspect, float aNear, float aFar)
{
    fov = aFov;
    aspect = aAspect;
    near = aNear;
    far = aFar;
    CalcProjection();
}
    

void Camera::CalcProjection()
{
    // If fails, has Init been called?
    SDL_assert(fov != 0.0 && aspect != 0.0 && near != 0.0 && far != 0.0);
    projection = glm::perspective(fov, aspect, near, far);
}

void Camera::SetFovAndRecalcProjection(float aFov)
{
    if (aFov != fov) {
        fov = aFov;
        CalcProjection();
    }
}


/*
void Camera::SetFov(float aFov)
{
    if (aFov != fov) {
        fov = aFov;
        CalcProjection(
*/



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
    dir = glm::normalize(dir);
    glm::vec3 posOffset = dir * dist;
    pos = targ - posOffset;
}


void Camera::SetFollowSmooth(float yaw, float pitch, float dist, glm::vec3 targ, double angleSmoothing, double distSmoothing)
{
    glm::vec3 hDir = dir;
    hDir.y = 0;
    hDir = glm::normalize(hDir);

    float currYaw = SDL_PI_F - SDL_atan2f(hDir.z, -hDir.x);

    // Do an angular lerp. 
    // This could be a more simple solution: 
    // https://stackoverflow.com/a/30129248/20829099

    angleSmoothing = SDL_min(angleSmoothing, 1.0);
    distSmoothing = SDL_min(distSmoothing, 1.0);
    
    float max = SDL_PI_F * 2.0f;
    float da = SDL_fmodf(yaw - currYaw, max);
    float angleDist = SDL_fmodf(2*da, max) - da;
    float newYaw = currYaw + angleDist * angleSmoothing;

    float currDist = glm::length(targ - pos);
    float newDist = currDist + (dist - currDist) * distSmoothing;
    SetFollow(newYaw, pitch, newDist, targ);
}

void Camera::SetFollowSmooth(float yaw, float pitch, float dist, glm::vec3 targ, double smoothing)
{
    SetFollowSmooth(yaw, pitch, dist, targ, smoothing, smoothing);
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
