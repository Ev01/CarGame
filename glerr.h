#pragma once
#include "glad/glad.h"

#define GLERR do {\
        GLuint glerr;\
        while((glerr = glGetError()) != GL_NO_ERROR)\
            SDL_Log("%s:%d glGetError() = 0x%04x", __FILE__, __LINE__, glerr);\
    } while (0)
