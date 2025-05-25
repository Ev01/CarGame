#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include "glad/glad.h"
#include "texture.h"

Texture gDefaultTexture;

Texture CreateTextureFromFile(const char* filename) {
    unsigned int textureId;
    Texture texture;
    // Init ID to prevent garbage values
    SDL_Surface *surf = IMG_Load(filename);
    if (surf == NULL) {
        SDL_Log("Could not load texture %s: %s", filename, SDL_GetError());
        return texture;
    }

    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
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

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surf->w, surf->h, 0,
                 format, GL_UNSIGNED_BYTE, surf->pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    SDL_free(surf);
    texture.id = textureId;
    return texture;
}


Texture CreateBlankTexture()
{
    SDL_Surface *surf = SDL_CreateSurface(1, 1, SDL_PIXELFORMAT_RGB24);
    // Fill surface with white
    SDL_FillSurfaceRect(surf, NULL, 0xFFFFFF);
    unsigned int textureId;

    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 surf->pixels);
    SDL_free(surf);
    Texture texture;
    texture.id = textureId;
    return texture;
}

void InitDefaultTexture()
{
    gDefaultTexture = CreateBlankTexture();
    SDL_Log("Default texture id = %d", gDefaultTexture.id);
}
