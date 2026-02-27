#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>

class WorleyNoise3D
{
public:
    WorleyNoise3D(int numCells);
    std::vector<glm::vec4>& getPoints();
    void GeneratePoints();

private:
    int _resolutionX, _resolutionY, _resolutionZ;
    int numCells;

    std::vector<glm::vec4> featurePoints; 
    std::vector<float> volumeData;        

};
