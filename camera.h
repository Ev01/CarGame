#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


struct Camera {
    glm::vec3 pos;
    // May not be normalised
    glm::vec3 dir;

    glm::vec3 Target();
    glm::vec3 Right(glm::vec3 up);
    void SetYawPitch(float yaw, float pitch);
    void SetFollow(float yaw, float pitch, float dist, glm::vec3 targ);
    void SetFollowSmooth(float yaw, float pitch, float dist, glm::vec3 targ, float smoothing);
    void SetOrbit(float dist, float height, glm::vec3 targ);
    glm::mat4 LookAtMatrix(glm::vec3 up);
    //void rotate(float angle, glm::vec3 axis);

    //glm::vec3 direction();
};

