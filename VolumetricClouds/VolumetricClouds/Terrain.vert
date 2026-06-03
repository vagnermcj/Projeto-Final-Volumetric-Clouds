#version 410 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 3) in vec2 aUV;

out vec3 FragPos;
out vec3 Normal;
out vec2 UV;

uniform mat4 model;
uniform mat4 camMatrix;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos       = worldPos.xyz;
    Normal        = normalize(mat3(transpose(inverse(model))) * aNormal);
    UV            = aUV;

    gl_Position = camMatrix * worldPos;
}
