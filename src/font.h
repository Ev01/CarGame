#pragma once

#include "texture.h"

#include <glm/glm.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

//extern FT_Library ft;
//extern FT_Face face;

// Single character for a font
struct Character {
    Texture      texture;
    glm::ivec2   size    = glm::ivec2(0.0);
    glm::ivec2   bearing = glm::ivec2(0.0);
    unsigned int advance = 0;
};

//extern Character characters[96];


namespace Font {
    struct Face {
        Character characters[96];
        FT_Face ftFace;
        float GetWidthOfText(const char* text, int textLength);
        float GetLineHeight();
    };

    extern Face *defaultFace;

    bool Init();
    Face* CreateFaceFromFile(const char* filename);
    void CleanUp();
}

