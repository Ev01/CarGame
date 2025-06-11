#pragma once

#include <SDL3/SDL.h>

struct Texture {
    Texture();
    ~Texture();
    void Destroy();
    unsigned int id = 0;
};

extern Texture gDefaultTexture;
extern Texture gDefaultNormalMap;

Texture CreateTextureFromFile(const char* filename);
Texture CreateTextureFromFile(const char* filename, bool isSRGB);
Texture CreateCubemapFromFiles(const char* fileRight, const char* fileLeft, 
                               const char* fileTop, const char* fileBottom, 
                               const char* fileBack, const char* fileFront);
Texture CreateBlankTexture(Uint32 colour = 0xFFFFFF);
void InitDefaultTexture();
