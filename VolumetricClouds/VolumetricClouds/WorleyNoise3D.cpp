#include "WorleyNoise3D.h"
#include <random>
#include <algorithm>
#include <cmath>


WorleyNoise3D::WorleyNoise3D(int resolutionX, int resolutionY, int resolutionZ,
    int numCells) : _resolutionX(resolutionX), _resolutionY(resolutionY),
    _resolutionZ(resolutionZ), numCells(numCells)
{
   volumeData.resize(_resolutionX * _resolutionY * _resolutionZ);
   featurePoints.resize(numCells * numCells * numCells);
}

glm::vec3 WorleyNoise3D::RandomPointInCell(int x, int y, int z)
{
    static std::mt19937 rng(1337);
    static std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    return glm::vec3(
        x + dist(rng),
        y + dist(rng),
        z + dist(rng)
    );
}

void WorleyNoise3D::Generate()
{
    // 1. Gerar pontos apenas para a grade de células (numCells)
    for (int z = 0; z < numCells; z++)
        for (int y = 0; y < numCells; y++)
            for (int x = 0; x < numCells; x++)
            {
                int idx = x + y * numCells + z * numCells * numCells;
                featurePoints[idx] = RandomPointInCell(x, y, z);
            }

    // 2. Preencher a textura pixel por pixel (resolution)
    for (int z = 0; z < _resolutionZ; ++z)
    {
        for (int y = 0; y < _resolutionY; ++y)
        {
            for (int x = 0; x < _resolutionX; ++x)
            {
                float u = (float)x / _resolutionX;
                float v = (float)y / _resolutionY;
                float w = (float)z / _resolutionZ;

                glm::vec3 p = glm::vec3(u, v, w) * (float)numCells;

                float d = ComputeWorleyValue(p);

                int idx = x + (y * _resolutionX) + (z * _resolutionX * _resolutionY);
                volumeData[idx] = d;
            }
        }
    }

}

float WorleyNoise3D::ComputeWorleyValue(const glm::vec3& p)
{
    int px = (int)glm::floor(p.x);
    int py = (int)glm::floor(p.y);
    int pz = (int)glm::floor(p.z);

    float minDist = 100.0f; 

    for (int dz = -1; dz <= 1; dz++)
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++)
            {
                int nx = px + dx;
                int ny = py + dy;
                int nz = pz + dz;

                int wrappedX = (nx % numCells + numCells) % numCells;
                int wrappedY = (ny % numCells + numCells) % numCells;
                int wrappedZ = (nz % numCells + numCells) % numCells;

                int idx = wrappedX + wrappedY * numCells + wrappedZ * numCells * numCells;
                glm::vec3 fp = featurePoints[idx];

               glm::vec3 offset(dx, dy, dz);

  
                glm::vec3 featurePointInCell = fp - glm::floor(fp);
                glm::vec3 trueNeighborPoint = glm::vec3(nx, ny, nz) + featurePointInCell;

                float dist = glm::distance(p, trueNeighborPoint);
                minDist = std::min(minDist, dist);
            }

    return glm::clamp(minDist, 0.0f, 1.0f);
}

std::vector<float> WorleyNoise3D::getData()
{
    return volumeData;
}
