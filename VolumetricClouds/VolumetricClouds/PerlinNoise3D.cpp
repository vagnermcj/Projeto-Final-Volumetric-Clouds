#include "PerlinNoise3D.h"
#include <cmath>

PerlinNoise3D::PerlinNoise3D(int resolutionX, int resolutionY, int resolutionZ, int numCells): _resolutionX(resolutionX), _resolutionY(resolutionY),
_resolutionZ(resolutionZ), numCells(numCells)
{
    volumeData.resize(_resolutionX * _resolutionY * _resolutionZ);
    InitPermutation();
}

void PerlinNoise3D::InitPermutation()
{
    int permutation[256] = {
        151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,
        140,36,103,30,69,142,8,99,37,240,21,10,23,190, 6,148,247,
        120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,
        177,33,88,237,149,56,87,174,20,125,136,171,168, 68,175,
        74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,
        122,60,211,133,230,220,105,92,41,55,46,245,40,244,102,
        143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208,
        89,18,169,200,196,135,130,116,188,159,86,164,100,109,
        198,173,186, 3,64,52,217,226,250,124,123, 5,202,38,147,
        118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
        189,28,42,223,183,170,213,119,248,152, 2,44,154,163,70,
        221,153,101,155,167, 43,172,9,129,22,39,253, 19,98,108,
        110,79,113,224,232,178,185,112,104,218,246,97,228,251,
        34,242,193,238,210,144,12,191,179,162,241, 81,51,145,
        235,249,14,239,107,49,192,214, 31,181,199,106,157,184,
        84,204,176,115,121, 50,45,127, 4,150,254,138,236,205,
        93,222,114,67,29,24,72,243,141,128,195,78,66,215, 61,
        156,180
    };

    for (int i = 0; i < 256; i++)
    {
        p[i] = permutation[i];
        p[256 + i] = permutation[i];
    }
}



float mixFloat(float a, float b, float t)
{
    return a * (1.0f - t) + b * t;
}


float PerlinNoise3D::Fade(float t)
{
    return t * t * t * (t * (t * 6 - 15) + 10);
}

float PerlinNoise3D::Grad(int hash, float x, float y, float z)
{
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : ((h == 12 || h == 14) ? x : z);
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

float PerlinNoise3D::Perlin3D(float x, float y, float z, int numCells)
{
    int xi = (int)floor(x);
    int yi = (int)floor(y);
    int zi = (int)floor(z);

    float xf = x - floor(x);
    float yf = y - floor(y);
    float zf = z - floor(z);

    float u = Fade(xf);
    float v = Fade(yf);
    float w = Fade(zf);

    // X
    int x0 = xi % numCells; if (x0 < 0) x0 += numCells; 
    int x1 = (x0 + 1) % numCells;                     

    // Y
    int y0 = yi % numCells; if (y0 < 0) y0 += numCells;
    int y1 = (y0 + 1) % numCells;

    // Z
    int z0 = zi % numCells; if (z0 < 0) z0 += numCells;
    int z1 = (z0 + 1) % numCells;

    x0 &= 255; x1 &= 255;
    y0 &= 255; y1 &= 255;
    z0 &= 255; z1 &= 255;

    int A = p[x0] + y0;
    int AA = p[A] + z0;
    int AB = p[A + 1] + z0;
    int B = p[x1] + y0;
    int BA = p[B] + z0;
    int BB = p[B + 1] + z0;

    int px0 = p[x0];
    int px1 = p[x1];

    int py0_0 = p[px0 + y0];
    int py1_0 = p[px1 + y0];
    int py0_1 = p[px0 + y1];
    int py1_1 = p[px1 + y1];

    int aaa = p[py0_0 + z0];
    int aba = p[py0_1 + z0];
    int aab = p[py0_0 + z1];
    int abb = p[py0_1 + z1];

    int baa = p[py1_0 + z0];
    int bba = p[py1_1 + z0];
    int bab = p[py1_0 + z1];
    int bbb = p[py1_1 + z1];

    float res = mixFloat(
        mixFloat(
            mixFloat(Grad(aaa, xf, yf, zf), Grad(baa, xf - 1, yf, zf), u),
            mixFloat(Grad(aba, xf, yf - 1, zf), Grad(bba, xf - 1, yf - 1, zf), u),
            v),
        mixFloat(
            mixFloat(Grad(aab, xf, yf, zf - 1), Grad(bab, xf - 1, yf, zf - 1), u),
            mixFloat(Grad(abb, xf, yf - 1, zf - 1), Grad(bbb, xf - 1, yf - 1, zf - 1), u),
            v),
        w);

    return res * 0.5f + 0.5f;
}


void PerlinNoise3D::Generate()
{

    for (int z = 0; z < _resolutionZ; z++)
        for (int y = 0; y < _resolutionY; y++)
            for (int x = 0; x < _resolutionX; x++)
            {
                float nx = float(x) / _resolutionX;
                float ny = float(y) / _resolutionY;
                float nz = float(z) / _resolutionZ;

                float n = Perlin3D(nx * numCells , ny * numCells, nz * numCells, numCells);

                volumeData[x + y * _resolutionX + (z * _resolutionX * _resolutionY)] = n; 
            }
}

std::vector<float> PerlinNoise3D::getData()
{
    return volumeData;
}