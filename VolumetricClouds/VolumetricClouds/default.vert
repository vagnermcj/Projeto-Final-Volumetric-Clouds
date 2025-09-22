#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;

out vec3 color;
out vec3 Normal;
out vec3 Pos;


uniform mat4 camMatrix;
uniform mat4 model;

void main()
{
	gl_Position = camMatrix * vec4(aPos,1.0);
	color = aColor;
	Normal = aNormal;
	Pos = aPos;
}