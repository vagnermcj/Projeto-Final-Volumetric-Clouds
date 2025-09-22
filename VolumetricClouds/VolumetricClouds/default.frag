#version 410 core
in vec3 color;
in vec3 Normal;
in vec3 Pos;

out vec4 FragColor;

uniform vec4 lightColor;
uniform vec3 lightDirection;
uniform vec3 camPos;


void main()
{
	float ambientLight = 0.2f;
	vec3 normal = normalize(Normal);
	vec3 lightDir = normalize(lightDirection);

	float diffuse = max(dot(normal, lightDir), 0.0f);

	// specular lighting
	float specularLight = 0.50f;
	vec3 viewDirection = normalize(camPos - Pos);
	vec3 reflectionDirection = reflect(-lightDir, normal);
	float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 8);
	float specular = specAmount * specularLight;

	FragColor =  vec4(color, 1.0f) * lightColor * (diffuse + ambientLight + specular);
}