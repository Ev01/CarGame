#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D spotLightShadowMapAtlas;
//uniform sampler2D tex;

const float near = 0.2f;
const float far = 40.0f;

void main()
{
    int shadowMapNum = 0;
    float x = (TexCoords.x + shadowMapNum) / 8.0;
    vec2 uv = vec2(x, TexCoords.y);
    float depth = texture(spotLightShadowMapAtlas, uv).r;
    //float depth = texture(tex, TexCoords).r;
    float ndc = depth * 2.0 - 1.0;
    float linearDepth = (2.0 * near * far) / (far + near - ndc * (far - near));
    float v = linearDepth;
    //FragColor = vec4(v, v, v, 1.0);
    FragColor = vec4(depth, depth, depth, 1.0);
    //FragColor = vec4(uv, 1.0, 1.0);
}
