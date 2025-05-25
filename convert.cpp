#include "convert.h"

glm::mat4 QuatToMatrix(JPH::Quat quat) {
    glm::quat glmQuat(quat.GetW(), quat.GetX(), quat.GetY(), quat.GetZ());
    glmQuat = glm::normalize(glmQuat);
    return glm::mat4_cast(glmQuat);
}


JPH::Vec3 ToJoltVec3(glm::vec3 v)
{
    JPH::Vec3 jVec(v.x, v.y, v.z);
    return jVec;
}

JPH::Vec3 ToJoltVec3(aiVector3D v)
{
    JPH::Vec3 jVec(v.x, v.y, v.z);
    return jVec;
}


glm::vec3 ToGlmVec3(JPH::Vec3 v)
{
    return glm::vec3(v.GetX(), v.GetY(), v.GetZ());
}


glm::mat4 ToGlmMat4(JPH::RMat44 joltMat)
{
    JPH::Vec4 col1 = joltMat.GetColumn4(0);
    JPH::Vec4 col2 = joltMat.GetColumn4(1);
    JPH::Vec4 col3 = joltMat.GetColumn4(2);
    JPH::Vec4 col4 = joltMat.GetColumn4(3);
    glm::mat4 gMat = {
        {col1.GetX(), col1.GetY(), col1.GetZ(), col1.GetW()},
        {col2.GetX(), col2.GetY(), col2.GetZ(), col2.GetW()},
        {col3.GetX(), col3.GetY(), col3.GetZ(), col3.GetW()},
        {col4.GetX(), col4.GetY(), col4.GetZ(), col4.GetW()}
    };
    return gMat;
}

glm::mat4 ToGlmMat4(aiMatrix4x4 aiMat)
{
    // Assimp's matrices are stored in row-major format, as opposed to glm
    // matrices which are column-major. Therefore, the matrix is transposed when
    // converting. Note that in assimp matrices, letters indicate rows and
    // numbers indicate columns, so a1, a2, a3, a4 is the first row.
    glm::mat4 gMat = {
        {aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1},
        {aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2},
        {aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3},
        {aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4},
    };
    return gMat;
}

JPH::RMat44 ToJoltMat4(aiMatrix4x4 aiMat)
{
    JPH::RMat44 joltMat (
         JPH::Vec4(aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1),
         JPH::Vec4(aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2),
         JPH::Vec4(aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3),
         JPH::Vec4(aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4)
         );
    return joltMat;
}
