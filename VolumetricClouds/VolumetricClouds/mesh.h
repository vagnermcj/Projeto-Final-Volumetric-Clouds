#include <memory>
class Mesh;
using MeshPtr = std::shared_ptr<Mesh>;

#ifndef MESH_H
#define MESH_H

#include <string>
#include <vector>

struct SubMesh {
    unsigned int textureID;   // 0 = sem textura
    unsigned int indexOffset; // em número de índices (não bytes)
    unsigned int indexCount;
};

class Mesh
{
    unsigned int m_vao;
    unsigned int m_nind;
    std::vector<SubMesh> m_submeshes;

protected:
    Mesh(const std::string& filename);
    Mesh();

public:
    static MeshPtr Make(const std::string& filename);
    static MeshPtr Make();
    virtual ~Mesh();

    // Usado pelo main quando não há MTL (fallback de textura única)
    void SetFallbackTexture(unsigned int textureID);

    void SetCoordBuffer(int size, const float* data, int ncomp, int stride);
    void SetNormalBuffer(int size, const float* data, int ncomp, int stride);
    void SetTangentBuffer(int size, const float* data, int ncomp, int stride);
    void SetTexCoordBuffer(int size, const float* data, int ncomp, int stride);
    void SetIndexBuffer(int size, const unsigned int* data);

    virtual void Draw();
};
#endif