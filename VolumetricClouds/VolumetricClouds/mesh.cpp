#include "mesh.h"

#ifdef _WIN32
//#define GLAD_GL_IMPLEMENTATION // Necessary for headeronly version.
#include <glad/glad.h>
#elif __APPLE__
#include <OpenGL/gl3.h>
#endif

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <map>
#include <tuple>
#include <glm/glm.hpp>

MeshPtr Mesh::Make (const std::string& filename)
{
  return MeshPtr(new Mesh(filename));
}

MeshPtr Mesh::Make ()
{
  return MeshPtr(new Mesh());
}

Mesh::Mesh(const std::string& filename)
{
    std::vector<glm::vec3> temp_positions;
    std::vector<glm::vec3> temp_normals;
    std::vector<glm::vec2> temp_uvs;

    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 uv;
        bool operator<(const Vertex& other) const {
            return memcmp(this, &other, sizeof(Vertex)) < 0;
        }
    };

    std::map<Vertex, unsigned int> vertexToIndex;
    std::vector<Vertex> finalVertices;
    std::vector<unsigned int> finalIndices;

    std::ifstream fp(filename);
    if (!fp) {
        std::cerr << "Could not open OBJ file: " << filename << std::endl;
        exit(1);
    }

    std::string line;
    while (std::getline(fp, line)) {
        std::stringstream ss(line);
        std::string type;
        ss >> type;

        if (type == "v") {
            glm::vec3 v;
            ss >> v.x >> v.y >> v.z;
            temp_positions.push_back(v);
        }
        else if (type == "vt") {
            glm::vec2 uv;
            ss >> uv.x >> uv.y;
            temp_uvs.push_back(uv);
        }
        else if (type == "vn") {
            glm::vec3 n;
            ss >> n.x >> n.y >> n.z;
            temp_normals.push_back(n);
        }
        else if (type == "f") {
            std::vector<std::string> tokens;
            std::string tok;
            while (ss >> tok)
                tokens.push_back(tok);

            if (tokens.size() < 3) continue;

            for (int k = 1; k < tokens.size() - 1; k++) {
                std::string vs[3] = { tokens[0], tokens[k], tokens[k + 1] };

                for (int i = 0; i < 3; i++) {
                    unsigned int vi = 0, ti = 0, ni = 0;

                    std::stringstream fs(vs[i]);
                    std::string s;
                    int idx = 0;
                    while (std::getline(fs, s, '/')) {
                        if (!s.empty()) {
                            if (idx == 0) vi = std::stoi(s) - 1;
                            else if (idx == 1) ti = std::stoi(s) - 1;
                            else if (idx == 2) ni = std::stoi(s) - 1;
                        }
                        idx++;
                    }

                    Vertex vert{};
                    vert.pos = temp_positions[vi];

                    vert.uv = (ti < temp_uvs.size()) ? temp_uvs[ti] : glm::vec2(0.0f);
                    vert.normal = (ni < temp_normals.size()) ? temp_normals[ni] : glm::vec3(0.0f);

                    if (vertexToIndex.count(vert) == 0) {
                        vertexToIndex[vert] = (unsigned int)finalVertices.size();
                        finalVertices.push_back(vert);
                    }

                    finalIndices.push_back(vertexToIndex[vert]);
                }
            }
        }
    }
    fp.close();

    if (temp_normals.empty()) {
        std::cout << "[OBJ loader] Warning: OBJ has no normals. Generating flat normals.\n";

        std::vector<glm::vec3> genNormals(finalVertices.size(), glm::vec3(0));

        for (int i = 0; i < finalIndices.size(); i += 3) {
            unsigned int i0 = finalIndices[i];
            unsigned int i1 = finalIndices[i + 1];
            unsigned int i2 = finalIndices[i + 2];

            glm::vec3 a = finalVertices[i1].pos - finalVertices[i0].pos;
            glm::vec3 b = finalVertices[i2].pos - finalVertices[i0].pos;
            glm::vec3 n = glm::normalize(glm::cross(a, b));

            genNormals[i0] += n;
            genNormals[i1] += n;
            genNormals[i2] += n;
        }

        for (int i = 0; i < finalVertices.size(); i++)
            finalVertices[i].normal = glm::normalize(genNormals[i]);
    }

    std::vector<float> coords;
    std::vector<float> normals;
    std::vector<float> texcoords;

    for (auto& v : finalVertices) {
        coords.push_back(v.pos.x);
        coords.push_back(v.pos.y);
        coords.push_back(v.pos.z);

        normals.push_back(v.normal.x);
        normals.push_back(v.normal.y);
        normals.push_back(v.normal.z);

        texcoords.push_back(v.uv.x);
        texcoords.push_back(v.uv.y);
    }

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    if (!coords.empty())
        SetCoordBuffer((int)coords.size(), coords.data(), 3, 0);

    if (!normals.empty())
        SetNormalBuffer((int)normals.size(), normals.data(), 3, 0);

    if (!texcoords.empty())
        SetTexCoordBuffer((int)texcoords.size(), texcoords.data(), 2, 0);

    SetIndexBuffer((int)finalIndices.size(), finalIndices.data());
}


Mesh::Mesh () 
{
  glGenVertexArrays(1,&m_vao);
}

Mesh::~Mesh () 
{
}

void Mesh::SetCoordBuffer (int size, const float* data, int ncomp, int stride)
{
  glBindVertexArray(m_vao);
  // create coord buffer
  GLuint id;
  glGenBuffers(1,&id);
  glBindBuffer(GL_ARRAY_BUFFER,id);
  glBufferData(GL_ARRAY_BUFFER,size*sizeof(float),(void*)data,GL_STATIC_DRAW);
  glVertexAttribPointer(0,ncomp,GL_FLOAT,GL_FALSE,stride,0);
  glEnableVertexAttribArray(0);
}

void Mesh::SetNormalBuffer (int size, const float* data, int ncomp, int stride)
{
  glBindVertexArray(m_vao);
  // create coord buffer
  GLuint id;
  glGenBuffers(1,&id);
  glBindBuffer(GL_ARRAY_BUFFER,id);
  glBufferData(GL_ARRAY_BUFFER,size*sizeof(float),(void*)data,GL_STATIC_DRAW);
  glVertexAttribPointer(1,ncomp,GL_FLOAT,GL_FALSE,stride,0);
  glEnableVertexAttribArray(1);
}

void Mesh::SetTangentBuffer (int size, const float* data, int ncomp, int stride)
{
  glBindVertexArray(m_vao);
  // create coord buffer
  GLuint id;
  glGenBuffers(1,&id);
  glBindBuffer(GL_ARRAY_BUFFER,id);
  glBufferData(GL_ARRAY_BUFFER,size*sizeof(float),(void*)data,GL_STATIC_DRAW);
  glVertexAttribPointer(2,ncomp,GL_FLOAT,GL_FALSE,stride,0);
  glEnableVertexAttribArray(2);
}

void Mesh::SetTexCoordBuffer (int size, const float* data, int ncomp, int stride)
{
  glBindVertexArray(m_vao);
  // create coord buffer
  GLuint id;
  glGenBuffers(1,&id);
  glBindBuffer(GL_ARRAY_BUFFER,id);
  glBufferData(GL_ARRAY_BUFFER,size*sizeof(float),(void*)data,GL_STATIC_DRAW);
  glVertexAttribPointer(3,ncomp,GL_FLOAT,GL_FALSE,stride,0);
  glEnableVertexAttribArray(3);
}

void Mesh::SetIndexBuffer (int size, const unsigned int* data)
{
  glBindVertexArray(m_vao);
  GLuint id;
  glGenBuffers(1,&id);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,id);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,size*sizeof(unsigned int),(void*)data,GL_STATIC_DRAW);
  m_nind = size;
}

void Mesh::Draw()
{
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_nind, GL_UNSIGNED_INT, 0);
}