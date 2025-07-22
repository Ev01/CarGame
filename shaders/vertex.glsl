#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitTangent;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

out VS_OUT {
    //vec3 Normal;
    vec3 FragPos;
    vec2 TexCoords;
    mat3 TBN;
    vec4 FragPosLightSpace;
} vs_out;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0f);
    vs_out.FragPos = vec3(view * model * vec4(aPos, 1.0f));
    //vs_out.Normal = mat3(transpose(inverse(view * model))) * aNormal;
    vs_out.TexCoords = aTexCoords;
    vs_out.FragPosLightSpace = lightSpaceMatrix * model * vec4(aPos, 1.0);

    vec3 T = normalize(vec3(view * model * vec4(aTangent,    0.0)));
    vec3 B = normalize(vec3(view * model * vec4(aBitTangent, 0.0)));
    vec3 N = normalize(vec3(view * model * vec4(aNormal,     0.0)));
    //vec3 B = cross(N, T);

    vs_out.TBN = mat3(T, B, N);
}

