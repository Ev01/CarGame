#pragma once

struct Texture {
    unsigned int id;
};

extern Texture gDefaultTexture;

Texture CreateTextureFromFile(const char* filename);
Texture CreateCubemapFromFiles(const char* fileRight, const char* fileLeft, 
                               const char* fileTop, const char* fileBottom, 
                               const char* fileBack, const char* fileFront);
Texture CreateBlankTexture();
void InitDefaultTexture();
