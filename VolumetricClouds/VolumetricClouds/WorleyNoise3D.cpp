// WorleyNoise3D.cpp - Versão simplificada para Compute Shader
#include "WorleyNoise3D.h"
#include <random>

WorleyNoise3D::WorleyNoise3D(int numCells) : numCells(numCells) {
    // Apenas redimensiona o vetor de pontos
    featurePoints.resize(numCells * numCells * numCells);
}

void WorleyNoise3D::GeneratePoints() {
    static std::mt19937 rng(1337);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    for (int z = 0; z < numCells; z++) {
        for (int y = 0; y < numCells; y++) {
            for (int x = 0; x < numCells; x++) {
                int idx = x + numCells * (y + z * numCells);
                // Ponto aleatório relativo à sua célula (0.0 a 1.0)
                featurePoints[idx] = glm::vec3(dist(rng), dist(rng), dist(rng));
            }
        }
    }
}

// Retorna o vetor para ser enviado ao SSBO
std::vector<glm::vec3>& WorleyNoise3D::getPoints() {
    return featurePoints;
}