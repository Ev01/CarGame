#version 330 core

#define MAX_SPOT_SHADOWS 16

in VS_OUT {
    //in vec3 Normal;
    in vec3 FragPos;
    in vec2 TexCoords;
    in mat3 TBN;
    in vec4 FragPosLightSpace;
    in vec4 FragPosSpotLightSpace[MAX_SPOT_SHADOWS];
} fs_in;

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



struct SpotLight {
    vec3 position;
    vec3 direction;

    vec3 diffuse;
    vec3 specular;

    float cutoffInner;
    float cutoffOuter;
    float quadratic;
    sampler2D shadowMap;
};



struct Material {
    sampler2D albedo;
    sampler2D roughnessMap;
    sampler2D normalMap;
    vec3 baseColour;
    float roughness;
    float metallic;
};

//uniform sampler2D diffuse;

uniform Material material;
uniform DirLight dirLight;

#define NUM_POINT_LIGHTS 16
#define NUM_SPOT_LIGHTS 16
uniform PointLight pointLights[NUM_POINT_LIGHTS];
uniform SpotLight spotLights[NUM_SPOT_LIGHTS];
uniform sampler2D shadowMap;

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec4 fragPosLightSpace);

#define PI 3.14159265359

float ShadowCalculation(sampler2D shadMap, vec4 fragPosLightSpace, float bias)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // Map [-1, 1] range to [0, 1] range
    projCoords = projCoords * 0.5 + 0.5;

    // Get depth of current fragment from lights perspective
    float currentDepth = projCoords.z;
    
    // Get cloest depth from lights perspective
    //float closestDepth = texture(shadMap, projCoords.xy).r;
    //float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadMap, 0);
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float pcfDepth = texture(shadMap,
                                     projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    

    return shadow;
}

void main() {
    //vec3 norm = normalize(Normal);

    vec4 textureSample = texture(material.albedo, fs_in.TexCoords);
    /*
    if (textureSample.a < 0.1) {
        discard;
    }
    */

    vec3 norm = texture(material.normalMap, fs_in.TexCoords).rgb;
    // RGB is from 0.0 to 1.0. Normals coords should be from -1.0 to 1.0.
    norm = norm * 2.0 - 1.0;
    norm = normalize(fs_in.TBN * norm);

    vec3 viewDir = normalize(-fs_in.FragPos);
    

    // Outgoing radiance
    vec3 Lo = vec3(0.0);
    Lo += CalcDirLight(dirLight, norm, viewDir);
    
    
    for (int i = 0; i < NUM_POINT_LIGHTS; i++) {
        Lo += CalcPointLight(pointLights[i], norm, fs_in.FragPos, viewDir);
    }
    
    
    for (int i = 0; i < NUM_SPOT_LIGHTS; i++) {
        Lo += CalcSpotLight(spotLights[i], norm, fs_in.FragPos, viewDir,
                fs_in.FragPosSpotLightSpace[i]);
    }

    
    vec3 albedo = material.baseColour * textureSample.rgb;
    vec3 ambient = vec3(0.01) * albedo;
    vec3 result  = ambient + Lo;

    //vec4 result = texture(diffuse, fs_in.TexCoords);
    // The alpha is quite hacky here.
    FragColor = vec4(result, textureSample.a);
    
    //vec3 normCol = (norm + 1.0) / 2.0;
    //FragColor = vec4(normCol, 1.0);
}


vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    // Calculate ratio of how much light is reflected vs refracted.
    // cosTheta: cosine of the angle between surface normal and view
    // F0: Surface reflection at zero incidence
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}


float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}


float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}


float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}


vec3 CalcLightIntensity(vec3 normal, vec3 lightDir, vec3 viewDir,
                        vec3 radiance)
{
    vec3 halfwayDir = normalize(lightDir + viewDir);

    vec3 albedo = material.baseColour 
                * texture(material.albedo, fs_in.TexCoords).rgb;
    float metallic = material.metallic;
    float roughness = texture(material.roughnessMap, fs_in.TexCoords).r 
                    * material.roughness;


    // Value of 0.04 approximates most dielectric surfaces
    vec3 F0 = vec3(0.04);
    F0      = mix(F0, albedo, metallic);
    vec3 F  = FresnelSchlick(max(dot(halfwayDir, viewDir), 0.0), F0);

    // Normal distribution function component
    float NDF = DistributionGGX(normal, halfwayDir, roughness);
    // Geometry component
    float G   = GeometrySmith(normal, viewDir, lightDir, roughness);

    // Cook-Torrance BRDF
    vec3 numerator    = NDF * G * F;
    // Add 0.0001 to prevent divide by zero
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) 
                            * max(dot(normal, lightDir), 0.0) + 0.0001;
    vec3 specular     = numerator / denominator;

    // Portion of light that is reflected (specular)
    vec3 kS = F;
    // Portion of light that is refracted (diffuse)
    vec3 kD = vec3(1.0) - kS;
    // Metallic objects have no diffuse lighting
    kD *= 1.0 - metallic;

    float NdotL = max(dot(normal, lightDir), 0.0);
    return (kD * albedo / PI + specular) * radiance * NdotL;
}




vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(-light.direction);
    // Calculate shadows
    //float shadowBias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    float shadowBias = -0.0005;
    float shadow = ShadowCalculation(shadowMap, fs_in.FragPosLightSpace, shadowBias);
    vec3 radiance = light.diffuse * (1.0 - shadow);
    return CalcLightIntensity(normal, lightDir, viewDir, radiance);
}



vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    if (light.constant + light.linear + light.quadratic == 0.0) {
        return vec3(0.0);
    }
    float dist = length(fragPos - light.position);
    float attenuation = 1.0 / (dist * dist);
    vec3 radiance = light.diffuse * attenuation;

    vec3 lightDir = normalize(light.position - fragPos);

    return CalcLightIntensity(normal, lightDir, viewDir, radiance);
}


vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir,
        vec4 fragPosLightSpace)
{
    if (light.quadratic == 0.0) {
        return vec3(0.0);
    }
    float dist = length(fragPos - light.position);
    float attenuation = 1.0 / (dist * dist);

    vec3 lightDir = normalize(light.position - fragPos);
    float cutoffMult = smoothstep(light.cutoffOuter, light.cutoffInner, dot(lightDir, normalize(-light.direction)));

    // Calculate shadows
    float shadow = ShadowCalculation(light.shadowMap, fragPosLightSpace, 0.0);
    //return vec3(1.0 - shadow);

    vec3 radiance = light.diffuse * attenuation * cutoffMult * (1.0 - shadow);

    return CalcLightIntensity(normal, lightDir, viewDir, radiance);
}

