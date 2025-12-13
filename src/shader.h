#pragma once

struct ShaderProg {
    unsigned int id;

    void SetInt(char uniformName[], int value);
    void SetFloat(char uniformName[], float value);
    void SetMat4fv(char uniformName[], const float *matrix);
    void SetVec3(char uniformName[], const float *vec3);
    void SetVec3(char uniformName[], float x, float y, float z);
    void SetVec4(char uniformName[], const float *vec4);
    void SetVec4(char uniformName[], float x, float y, float z, float w);
};

unsigned int CreateShaderFromFile(const char* filename, const int shaderType);

ShaderProg CreateAndLinkShaderProgram(unsigned int vertexShader, 
                                      unsigned int fragmentShader);

