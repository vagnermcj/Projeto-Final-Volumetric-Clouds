#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>

class WorleyNoise3D
{
public:
    WorleyNoise3D(int resolutionX, int resolutionY, int resolutionZ,
        int numCells);
    void Generate();
    std::vector<float> getData();

private:
    int _resolutionX, _resolutionY, _resolutionZ;
    int numCells;

    std::vector<glm::vec3> featurePoints; 
    std::vector<float> volumeData;        

    glm::vec3 RandomPointInCell(int x, int y, int z);
    float ComputeWorleyValue(const glm::vec3& p);
};
