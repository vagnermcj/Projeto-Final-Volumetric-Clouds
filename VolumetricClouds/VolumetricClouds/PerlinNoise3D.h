#ifndef PERLIN_NOISE_3D_CLASS
#define PERLIN_NOISE_3D_CLASS

#include <vector>
#include <glad/glad.h>

class PerlinNoise3D
{
public:
    PerlinNoise3D(int resolutionX, int resolutionY, int resolutionZ);
    void Generate();
    std::vector<float> getData();

private:
    int _resolutionX, _resolutionY, _resolutionZ;
    std::vector<float> volumeData;
    float Perlin3D(float x, float y, float z);
    float Fade(float t);
    float Grad(int hash, float x, float y, float z);

    int p[512]; // Permutation table
    void InitPermutation();
};


#endif 

