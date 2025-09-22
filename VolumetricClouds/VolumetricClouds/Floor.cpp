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

    // Vértices do chão com posição, cor e normal
    GLfloat vertices[] =
    {
        //    POSIÇÃO              COR                NORMAL
        -half, height, -half,   0.47f, 0.47f, 0.47f,   0.0f, 1.0f, 0.0f,
         half, height, -half,   0.47f, 0.47f, 0.47f,   0.0f, 1.0f, 0.0f,
         half, height,  half,   0.47f, 0.47f, 0.47f,   0.0f, 1.0f, 0.0f,
        -half, height,  half,   0.47f, 0.47f, 0.47f,   0.0f, 1.0f, 0.0f
    };

    GLuint indices[] =
    {
        0, 1, 2,
        2, 3, 0
    };

    FloorVAO.Bind();

    VBO FloorVBO(vertices, sizeof(vertices));
    EBO FloorEBO(indices, sizeof(indices));

    FloorVAO.LinkAttrib(FloorVBO, 0, 3, GL_FLOAT, 9 * sizeof(float), (void*)0); //Coord
    FloorVAO.LinkAttrib(FloorVBO, 1, 3, GL_FLOAT, 9 * sizeof(float), (void*)(3 * sizeof(float))); //Color
    FloorVAO.LinkAttrib(FloorVBO, 2, 3, GL_FLOAT, 9 * sizeof(float), (void*)(6 * sizeof(float))); //Normals

    FloorVAO.Unbind();
    FloorVBO.Delete();
    FloorEBO.Delete();
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