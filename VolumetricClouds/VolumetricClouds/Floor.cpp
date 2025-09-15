#include "Floor.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include <glad/glad.h>

FloorPtr Floor::Make(float area, float height)
{
	return FloorPtr(new Floor(area, height));
}

Floor::Floor(float area, float height)
{
	float half = area / 2.0f;

	GLfloat vertices[] =
	{
		-half, height, -half,
		 half, height, -half,
		 half, height,  half,
		-half, height,  half
	};

	GLfloat color[] =
	{
		0.47f, 0.47f, 0.47f,
		0.47f, 0.47f, 0.47f,
		0.47f, 0.47f, 0.47f,
		0.47f, 0.47f, 0.47f
	};

	

	FloorVAO.Bind();

	VBO VertexVBO(vertices, sizeof(vertices));
	VBO ColorVBO(color, sizeof(color));

	EBO EBO1(indexes, sizeof(indexes));

	// Links VBO to VAO
	FloorVAO.LinkAttrib(VertexVBO, 0, 3, GL_FLOAT, 3 * sizeof(float), (void*)0); //Terminar, decidindo se usa um VBO soh ou dois
	FloorVAO.LinkAttrib(ColorVBO, 1, 3, GL_FLOAT, 3 * sizeof(float), (void*)0);

	FloorVAO.Unbind();
	VertexVBO.Delete();
	ColorVBO.Delete();
	EBO1.Delete();
}

void Floor::Draw()
{
	FloorVAO.Bind();
	glDrawElements(GL_TRIANGLES, sizeof(indexes) / sizeof(int), GL_UNSIGNED_INT, 0);
	FloorVAO.Unbind();
}

Floor::~Floor()
{
	FloorVAO.Delete();
}