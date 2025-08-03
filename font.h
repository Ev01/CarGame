#pragma once

#include "texture.h"

#include <glm/glm.hpp>
#include <map>

// Single character for a font
struct Character {
    Texture      texture;
    glm::ivec2   size;
    glm::ivec2   bearing;
    unsigned int advance;
};

