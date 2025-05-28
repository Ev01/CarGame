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


struct PointLight {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};


struct Material {
    sampler2D texture_diffuse;
    sampler2D texture_specular;
    vec3 diffuseColour;
    float shininess;
};

//uniform sampler2D diffuse;

uniform Material material;
uniform DirLight dirLight;

#define NUM_POINT_LIGHTS 6
uniform PointLight pointLights[NUM_POINT_LIGHTS];

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(-FragPos);
    
    vec3 result = CalcDirLight(dirLight, norm, viewDir);

    for (int i = 0; i < NUM_POINT_LIGHTS; i++) {
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);
    }

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


vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    float dist = length(fragPos - light.position);
    float attenuation = 1.0f / (light.constant + light.linear * dist
                                + light.quadratic * dist * dist);

    vec3 lightDir = normalize(light.position - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);

    float diff = max(dot(normal, lightDir), 0.0);
    float spec = pow(max(dot(reflectDir, viewDir), 0.0), material.shininess);


    vec3 ambient = vec3(texture(material.texture_diffuse, TexCoords))
                 * light.ambient * material.diffuseColour;
    vec3 diffuse = diff * vec3(texture(material.texture_diffuse, TexCoords))
                 * light.diffuse * material.diffuseColour;
    vec3 specular = spec * vec3(texture(material.texture_specular, TexCoords))
                 * light.diffuse;

    return (ambient + diffuse + specular) * attenuation;
}


