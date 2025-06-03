#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include "glad/glad.h"
#include "texture.h"

Texture gDefaultTexture;
Texture gDefaultNormalMap;

Texture CreateTextureFromFile(const char* filename, bool isSRGB) {
    unsigned int textureId;
    Texture texture;
    // Init ID to prevent garbage values
    texture.id = 0;
    SDL_Surface *surf = IMG_Load(filename);
    if (surf == NULL) {
        SDL_Log("Could not load texture %s: %s", filename, SDL_GetError());
        return texture;
    }

    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    GLenum format;
    switch (surf->format) {
        case SDL_PIXELFORMAT_RGBA32:
            format = GL_RGBA;
            break;
        case SDL_PIXELFORMAT_BGRA32:
            format = GL_BGRA;
            break;
        case SDL_PIXELFORMAT_RGB24:
            format = GL_RGB;
            break;
        case SDL_PIXELFORMAT_BGR24:
            format = GL_BGR;
            break;
        default:
            SDL_Log("Pixel format of file %s not recognised: Using default (RGB)", filename);
            format = GL_RGB;
            break;
    }

    GLenum targetFormat = isSRGB ? GL_SRGB : GL_RGB;

    glTexImage2D(GL_TEXTURE_2D, 0, targetFormat, surf->w, surf->h, 0,
                 format, GL_UNSIGNED_BYTE, surf->pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    SDL_free(surf);
    texture.id = textureId;
    return texture;
}

Texture CreateTextureFromFile(const char* filename) {
    return CreateTextureFromFile(filename, true);
}


Texture CreateBlankTexture(Uint32 colour)
{
    SDL_Surface *surf = SDL_CreateSurface(1, 1, SDL_PIXELFORMAT_RGB24);
    // Fill surface with white
    SDL_FillSurfaceRect(surf, NULL, colour);
    unsigned int textureId;

    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 surf->pixels);
    SDL_free(surf);
    Texture texture;
    texture.id = textureId;
    return texture;
}


static void LoadTextureToCubemap(const char* filename, int target)
{
    SDL_Surface *surf = IMG_Load(filename);
    if (surf == NULL) {
        SDL_Log("Could not load texture %s: %s", filename, SDL_GetError());
        return;
    }

    GLenum format;
    switch (surf->format) {
        case SDL_PIXELFORMAT_RGBA32:
            format = GL_RGBA;
            break;
        case SDL_PIXELFORMAT_BGRA32:
            format = GL_BGRA;
            break;
        case SDL_PIXELFORMAT_RGB24:
            format = GL_RGB;
            break;
        case SDL_PIXELFORMAT_BGR24:
            format = GL_BGR;
            break;
        default:
            SDL_Log("Pixel format of file %s not recognised: Using default (RGB)", filename);
            format = GL_RGB;
            break;
    }

    glTexImage2D(target, 0, GL_RGBA, surf->w, surf->h, 0,
                 format, GL_UNSIGNED_BYTE, surf->pixels);
    SDL_free(surf);
}



Texture CreateCubemapFromFiles(const char* fileRight, const char* fileLeft, 
                               const char* fileTop, const char* fileBottom, 
                               const char* fileBack, const char* fileFront)
{
    unsigned int textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    LoadTextureToCubemap(fileRight, GL_TEXTURE_CUBE_MAP_POSITIVE_X);
    LoadTextureToCubemap(fileLeft, GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
    LoadTextureToCubemap(fileTop, GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
    LoadTextureToCubemap(fileBottom, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
    LoadTextureToCubemap(fileBack, GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
    LoadTextureToCubemap(fileFront, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);

    Texture texture;
    texture.id = textureId;
    return texture;
}



void InitDefaultTexture()
{
    gDefaultTexture = CreateBlankTexture(0xFFFFFF);
    gDefaultNormalMap = CreateBlankTexture(0xFF7F7F);
    SDL_Log("Default texture id = %d", gDefaultTexture.id);
}
