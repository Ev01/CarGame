#pragma once

#include <assimp/defs.h>

// Forward declarations
struct aiLight;
struct aiNode;

template<typename TReal> class aiMatrix4x4t;
typedef aiMatrix4x4t<ai_real> aiMatrix4x4;

namespace World {
    void AssimpAddLight(const aiLight *aLight, const aiNode *aNode,
                        aiMatrix4x4 aTransform);

    void DestroyAllLights();
}
