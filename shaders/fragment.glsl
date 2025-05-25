#version 330 core

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

out vec4 FragColor;


struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};


struct Material {
    sampler2D texture_diffuse;
    sampler2D texture_specular;
    vec3 diffuseColour;
    float shininess;
};

//uniform sampler2D diffuse;
uniform DirLight dirLight;
uniform Material material;

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);

void main() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(-FragPos);
    
    vec3 result = CalcDirLight(dirLight, norm, viewDir);

    //vec4 result = texture(diffuse, TexCoords);
    FragColor = vec4(result, 1.0f);
}


vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(-light.direction);
    vec3 reflectDir = reflect(-lightDir, normal);

    float diff = max(dot(normal, lightDir), 0.0);
    float spec = pow(max(dot(reflectDir, viewDir), 0.0), material.shininess);

    vec3 ambient = vec3(texture(material.texture_diffuse, TexCoords)) 
        * light.ambient * material.diffuseColour;
    vec3 diffuse = diff * vec3(texture(material.texture_diffuse, TexCoords))
        * light.diffuse * material.diffuseColour;
    vec3 specular = spec * vec3(texture(material.texture_specular, TexCoords))
        * light.specular;

    return ambient + diffuse + specular;
}

