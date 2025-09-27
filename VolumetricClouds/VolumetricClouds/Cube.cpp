#include "Cube.h"


CubePtr Cube::Make()
{
    return CubePtr(new Cube());
}

Cube::Cube()
{
    GLfloat vertices[] =
    {
        //   POSIÇÃO           COR             NORMAL
        // Frente (z = +0.5)
        -0.5f, -0.5f,  0.5f,   1.0f,1.0f,1.0f,   0.0f, 0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,   1.0f,1.0f,1.0f,   0.0f, 0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,   1.0f,1.0f,1.0f,   0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,   1.0f,1.0f,1.0f,   0.0f, 0.0f, 1.0f,

        // Trás (z = -0.5)
        -0.5f, -0.5f, -0.5f,   1.0f,1.0f,1.0f,   0.0f, 0.0f,-1.0f,
         0.5f, -0.5f, -0.5f,   1.0f,1.0f,1.0f,   0.0f, 0.0f,-1.0f,
         0.5f,  0.5f, -0.5f,   1.0f,1.0f,1.0f,   0.0f, 0.0f,-1.0f,
        -0.5f,  0.5f, -0.5f,   1.0f,1.0f,1.0f,   0.0f, 0.0f,-1.0f,

        // Direita (x = +0.5)
         0.5f, -0.5f, -0.5f,   1.0f,1.0f,1.0f,   1.0f, 0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,   1.0f,1.0f,1.0f,   1.0f, 0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,   1.0f,1.0f,1.0f,   1.0f, 0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,   1.0f,1.0f,1.0f,   1.0f, 0.0f, 0.0f,

         // Esquerda (x = -0.5)
         -0.5f, -0.5f, -0.5f,   1.0f,1.0f,1.0f,  -1.0f, 0.0f, 0.0f,
         -0.5f, -0.5f,  0.5f,   1.0f,1.0f,1.0f,  -1.0f, 0.0f, 0.0f,
         -0.5f,  0.5f,  0.5f,   1.0f,1.0f,1.0f,  -1.0f, 0.0f, 0.0f,
         -0.5f,  0.5f, -0.5f,   1.0f,1.0f,1.0f,  -1.0f, 0.0f, 0.0f,

         // Topo (y = +0.5)
         -0.5f,  0.5f, -0.5f,   1.0f,1.0f,1.0f,   0.0f, 1.0f, 0.0f,
          0.5f,  0.5f, -0.5f,   1.0f,1.0f,1.0f,   0.0f, 1.0f, 0.0f,
          0.5f,  0.5f,  0.5f,   1.0f,1.0f,1.0f,   0.0f, 1.0f, 0.0f,
         -0.5f,  0.5f,  0.5f,   1.0f,1.0f,1.0f,   0.0f, 1.0f, 0.0f,

         // Fundo (y = -0.5)
         -0.5f, -0.5f, -0.5f,   1.0f,1.0f,1.0f,   0.0f,-1.0f, 0.0f,
          0.5f, -0.5f, -0.5f,   1.0f,1.0f,1.0f,   0.0f,-1.0f, 0.0f,
          0.5f, -0.5f,  0.5f,   1.0f,1.0f,1.0f,   0.0f,-1.0f, 0.0f,
         -0.5f, -0.5f,  0.5f,   1.0f,1.0f,1.0f,   0.0f,-1.0f, 0.0f
    };

    GLuint indices[] =
    {
        // Frente (z = +0.5)
        0, 2, 1,
        0, 3, 2,

        // Trás (z = -0.5)
        4, 5, 6,
        4, 6, 7,

        // Direita (x = +0.5)
        8, 9, 10,
        8, 10, 11,

        // Esquerda (x = -0.5)
        12, 14, 13,
        12, 15, 14,

        // Topo (y = +0.5)
        16, 17, 18,
        16, 18, 19,

        // Fundo (y = -0.5)
        20, 22, 21,
        20, 23, 22
    };



    CubeVAO.Bind();

    VBO CubeVBO(vertices, sizeof(vertices));
    EBO CubeEBO(indices, sizeof(indices));
    // Posição → layout 0
    CubeVAO.LinkAttrib(CubeVBO, 0, 3, GL_FLOAT, 9 * sizeof(float), (void*)0);

    // Cor → layout 1
    CubeVAO.LinkAttrib(CubeVBO, 1, 3, GL_FLOAT, 9 * sizeof(float), (void*)(3 * sizeof(float)));

    // Normal → layout 2
    CubeVAO.LinkAttrib(CubeVBO, 2, 3, GL_FLOAT, 9 * sizeof(float), (void*)(6 * sizeof(float)));

    CubeVAO.Unbind();
    CubeVBO.Delete();
    CubeEBO.Delete();
}

void Cube::Draw()
{
    CubeVAO.Bind();
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    CubeVAO.Unbind();
}

Cube::~Cube()
{
    CubeVAO.Delete();
}
