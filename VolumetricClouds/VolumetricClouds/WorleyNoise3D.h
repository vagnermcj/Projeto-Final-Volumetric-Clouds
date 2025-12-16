#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>

class WorleyNoise3D
{
public:
    WorleyNoise3D(int resolution, int numCells);
    void Generate();                   
    void Bind(GLuint unit);
    GLuint GetTextureID() const { return textureID; }

private:
    int resolution;                    
    int numCells;
    GLuint textureID;

    std::vector<glm::vec3> featurePoints; 
    std::vector<float> volumeData;        

    glm::vec3 RandomPointInCell(int x, int y, int z);
    float ComputeWorleyValue(const glm::vec3& p);
};
