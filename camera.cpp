#include "camera.h"

#include "physics.h"
#include "convert.h"

#include <glm/glm.hpp>
#include <SDL3/SDL.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Body/BodyInterface.h>


// Filter used to determine which objects the camera should move in front of.
class CameraObjectLayerFilter : public JPH::ObjectLayerFilter {
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inLayer) const override
    {
        return inLayer != Phys::Layers::NON_SOLID;
    }
} cameraObjectLayerFilter;


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


void VehicleCamera::SetFollowSmooth(float yaw, float pitch, float dist,
                                    double angleSmoothing,
                                    double distSmoothing)
{
    JPH::BodyInterface &bodyInterface = Phys::GetBodyInterface();
    JPH::Vec3 targJolt = bodyInterface.GetCenterOfMassPosition(targetBody->GetID());
    JPH::Quat rot = bodyInterface.GetRotation(targetBody->GetID());
    JPH::Vec3 targDir = rot.RotateAxisX();
    float targYaw = SDL_PI_F - SDL_atan2f(targDir.GetX(), targDir.GetZ());

    glm::vec3 targ = ToGlmVec3(targJolt);
    cam.SetFollowSmooth(targYaw + yaw, pitch, dist, targ, angleSmoothing, distSmoothing);
    
    // Cast ray for camera
    // Doing it this way still makes the camera clip through a little bit. To
    // fix this, a spherecast could be used with a radius equal to the camera's
    // near-plane clipping distance. This code should also be applied before 
    // smoothing. For this, the camera's target
    // position may need to be stored to separate this code from smoothing.
    
    JPH::Vec3 newCamPos;
    JPH::Vec3 camPosJolt = ToJoltVec3(cam.pos);
    JPH::Vec3 camToTarg = targJolt - camPosJolt;
    JPH::IgnoreSingleBodyFilter carBodyFilter(targetBody->GetID());

    bool hadHit = Phys::CastRay(
            targJolt - camToTarg/2.0, -camToTarg/2.0, newCamPos,
            { }, cameraObjectLayerFilter, carBodyFilter);

    if (hadHit) {
        cam.pos = ToGlmVec3(newCamPos);
    }
}



void VehicleCamera::Init(float aFov, float aAspect, float aNear, float aFar)
{
    cam.Init(aFov, aAspect, aNear, aFar);
}
