#pragma once

struct Texture {
    unsigned int id;
};

extern Texture gDefaultTexture;

Texture CreateTextureFromFile(const char* filename);
Texture CreateBlankTexture();
void InitDefaultTexture();
