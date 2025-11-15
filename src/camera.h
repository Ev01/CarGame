#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <SDL3/SDL.h>

namespace JPH {
    struct Body;
}

struct Camera {
    glm::vec3 pos = glm::vec3(0.0);
    // May not be normalised
    glm::vec3 dir = glm::vec3(1.0, 0.0, 0.0);
    float fov;
    float near;
    float far;
    float aspect;
    glm::mat4 projection = glm::mat4(1.0);

    void Init(float aFov, float aAspect, float aNear, float aFar);
    void CalcProjection();
    void SetFovAndRecalcProjection(float fov);

    glm::vec3 Target() const;
    glm::vec3 Right(glm::vec3 up) const;
    // Sets the aspect ratio of the camera and updates the projection matrix.
    void SetAspectRatio(float aAspect);
    void SetYawPitch(float yaw, float pitch);
    void SetFollow(float yaw, float pitch, float dist, glm::vec3 targ);
    void SetFollowSmooth(float yaw, float pitch, float dist, glm::vec3 targ, double smoothing);
    void SetFollowSmooth(float yaw, float pitch, float dist, glm::vec3 targ, double angleSmoothing, double distSmoothing);
    void SetOrbit(float dist, float height, glm::vec3 targ);
    glm::mat4 LookAtMatrix(glm::vec3 up) const;
    //void rotate(float angle, glm::vec3 axis);

    //glm::vec3 direction();
};


struct VehicleCamera {
    Camera cam;
    JPH::Body *targetBody = nullptr;

    void SetFollowSmooth(float yaw, float pitch, float dist, double angleSmoothing, double distSmoothing);
    void Init(float aFov, float aAspect, float aNear, float aFar);
};

