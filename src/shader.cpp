#include "shader.h"
#include "../glad/glad.h"
#include "glerr.h"

#include <SDL3/SDL.h>



unsigned int CreateShaderFromFile(const char* filename, const int shaderType) {
    char *shaderSource = (char*)SDL_LoadFile(filename, NULL);
    if (shaderSource == NULL) {
        SDL_Log("Could not load shader file %s: %s", filename, SDL_GetError());
    }
    unsigned int shaderId;
    shaderId = glCreateShader(shaderType);
    glShaderSource(shaderId, 1, &shaderSource, NULL);
    glCompileShader(shaderId);

    int success;
    char infoLog[512];
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shaderId, 512, NULL, infoLog);
        SDL_Log("Could not compile shader %s: %s", filename, infoLog);
    }

    return shaderId;
}

ShaderProg CreateAndLinkShaderProgram(unsigned int vertexShader, 
                                      unsigned int fragmentShader)
{
    unsigned int shaderProgId = glCreateProgram();
    glAttachShader(shaderProgId, vertexShader);
    glAttachShader(shaderProgId, fragmentShader);
    glLinkProgram(shaderProgId);

    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgId, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgId, 512, NULL, infoLog);
        SDL_Log("Shader program creation failed: %s", infoLog);
    }
    else {
        SDL_Log("Successfully created shader program %u", shaderProgId);
    }

    ShaderProg program;
    program.id = shaderProgId;
    return program;
}


void ShaderProg::SetInt(char uniformName[], int value)
{
    glUniform1i(glGetUniformLocation(id, uniformName), value);
}

void ShaderProg::SetFloat(char uniformName[], float value)
{
    glUniform1f(glGetUniformLocation(id, uniformName), value);
}

void ShaderProg::SetMat4fv(char uniformName[], const float *matrix)
{
    GLERR;
    GLint loc = glGetUniformLocation(id, uniformName); 
    GLERR;
    glUniformMatrix4fv(loc, 1, GL_FALSE, matrix);
    GLERR;
}


void ShaderProg::SetVec3(char uniformName[], const float *vec3)
{
    glUniform3fv(glGetUniformLocation(id, uniformName), 1, vec3);
}

void ShaderProg::SetVec3(char uniformName[], float x, float y, float z)
{
    glUniform3f(glGetUniformLocation(id, uniformName), x, y, z);
}

void ShaderProg::SetVec4(char uniformName[], float x, float y, float z, float w)
{
    glUniform4f(glGetUniformLocation(id, uniformName), x, y, z, w);
}
