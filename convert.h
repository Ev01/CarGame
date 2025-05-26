#pragma once

#include <Jolt/Jolt.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <assimp/matrix4x4.h>

glm::mat4 QuatToMatrix(JPH::Quat quat);
JPH::Vec3 ToJoltVec3(glm::vec3 v);
JPH::Vec3 ToJoltVec3(aiVector3D v);
JPH::Vec4 ToJoltVec4(glm::vec4 v);
glm::vec3 ToGlmVec3(JPH::Vec3 v);
glm::mat4 ToGlmMat4(JPH::RMat44 joltMat);
glm::mat4 ToGlmMat4(aiMatrix4x4 aiMat);

JPH::RMat44 ToJoltMat4(aiMatrix4x4 aiMat);
JPH::RMat44 ToJoltMat4(glm::mat4 gMat);
