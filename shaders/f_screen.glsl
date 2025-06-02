#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;

const float of = 1.0 / 300.0;
const float exposure = 0.05;


    
vec3 EdgeDetect();

void main()
{
    //vec4 col = vec4(EdgeDetect(), 1.0);
    vec3 col = texture(screenTexture, TexCoords).rgb;

    // reinhard tonemapping
    //col = col / (col + vec3(1.0));
    
    // exposure tonemapping
    col = vec3(1.0) - exp(-col * exposure);

    // Gamma correction
    float gamma = 2.2;
    col = pow(col, vec3(1.0/gamma));

    FragColor = vec4(col, 1.0);
    //FragColor = vec4(0.0, 1.0, 0.0, 1.0);
}


vec3 EdgeDetect()
{
    vec2 offsets[9] = vec2[](
            vec2(-of, of),
            vec2(0.0f, of),
            vec2(of, of),
            vec2(-of, 0.0f),
            vec2(0.0f, 0.0f),
            vec2(of, 0.0f),
            vec2(-of, -of),
            vec2(0.0f, -of),
            vec2(of, -of)
    );

    float kernel[9] = float[](
            1, 1, 1,
            1, -8, 1,
            1, 1, 1
    );

    vec3 sampleTex[9];
    for (int i = 0; i < 9; i++) {
        sampleTex[i] = vec3(texture(screenTexture, TexCoords.st + offsets[i]));
    }
    vec3 col = vec3(0.0);
    for (int i = 0; i < 9; i++) {
        col += sampleTex[i] * kernel[i];
    }
    return col;
}
