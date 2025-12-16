#ifndef PERLIN_NOISE_3D_CLASS
#define PERLIN_NOISE_3D_CLASS

#include <vector>
#include <glad/glad.h>

class PerlinNoise3D
{
public:
    PerlinNoise3D(int size = 64);
    ~PerlinNoise3D();

    void Generate(float scale);
    void Bind(GLuint unit);

    GLuint GetID() const { return textureID; }
    int GetSize() const { return size; }

private:
    int size;
    GLuint textureID;

    float Perlin3D(float x, float y, float z);
    float Fade(float t);
    float Grad(int hash, float x, float y, float z);

    int p[512]; // Permutation table
    void InitPermutation();
};


#endif 

