#version 410 core

in vec3 FragPos;
in vec3 Normal;
in vec2 UV;

out vec4 FragColor;

uniform sampler2D terrainTex;
uniform bool      hasTexture;
uniform vec3 camPos;
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform vec3 ambientColor;
uniform float ambientIntensity;

// Cor base do terreno (pode trocar por uma textura depois)
uniform vec3 terrainColor;

void main()
{
    vec3 norm     = normalize(Normal);
    vec3 lightDir = normalize(lightDirection);

    // Difuso (Lambert)
    float diff   = max(dot(norm, lightDir), 0.0);
    vec3  diffuse = diff * lightColor;

    // Especular simples (Blinn-Phong)
    vec3  viewDir   = normalize(camPos - FragPos);
    vec3  halfDir   = normalize(lightDir + viewDir);
    float spec      = pow(max(dot(norm, halfDir), 0.0), 32.0);
    vec3  specular  = spec * lightColor * 0.1; // baixo para não brilhar demais no terreno

    // Ambient
    vec3 ambient = ambientColor * ambientIntensity * 0.15;

    vec3 baseColor = hasTexture ? texture(terrainTex, UV).rgb : terrainColor;
    vec3 result = (ambient + diffuse + specular) * baseColor;
    FragColor = vec4(result, 1.0);
}
