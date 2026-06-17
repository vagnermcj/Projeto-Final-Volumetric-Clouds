#include "mesh.h"

#ifdef _WIN32
#include <glad/glad.h>
#elif __APPLE__
#include <OpenGL/gl3.h>
#endif

#include "Libraries/include/stb/stb_image.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <unordered_map>
#include <tuple>
#include <glm/glm.hpp>

// ─── Utilitário ───────────────────────────────────────────────────────────────

static std::string dirOf(const std::string& path)
{
    size_t pos = path.find_last_of("/\\");
    return (pos == std::string::npos) ? "" : path.substr(0, pos + 1);
}

static GLuint loadTexture(const std::string& path)
{
    int w, h, ch;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &ch, 0);
    if (!data) {
        std::cerr << "[Mesh] Texture not found: " << path << "\n";
        return 0;
    }

    GLenum fmt = (ch == 4) ? GL_RGBA : GL_RGB;
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    stbi_image_free(data);

    std::cout << "[Mesh] Loaded texture: " << path << "\n";
    return id;
}

// ─── Parser de MTL ────────────────────────────────────────────────────────────
// Retorna mapa de nome-de-material -> textureID
// Prioridade: map_Kd (imagem) > Kd (cor sólida 1x1) > branco

static std::map<std::string, GLuint> parseMTL(const std::string& mtlPath)
{
    std::map<std::string, GLuint> result;
    std::ifstream f(mtlPath);
    if (!f) {
        std::cerr << "[Mesh] MTL not found: " << mtlPath << "\n";
        return result;
    }

    std::string dir = dirOf(mtlPath);
    std::string currentMat;
    std::string line;

    // Armazena Kd por material enquanto parseia
    std::map<std::string, glm::vec3> matKd;

    while (std::getline(f, line)) {
        std::stringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "newmtl") {
            ss >> currentMat;
            result[currentMat] = 0;
            matKd[currentMat] = glm::vec3(1.0f); // branco como default
        }
        else if (token == "Kd" && !currentMat.empty()) {
            glm::vec3 kd;
            ss >> kd.r >> kd.g >> kd.b;
            matKd[currentMat] = kd;
        }
        else if ((token == "map_Kd" || token == "map_kd") && !currentMat.empty()) {
            std::string texFile;
            ss >> texFile;
            while (!texFile.empty() && texFile[0] == '-') ss >> texFile;

            // Ignora caminhos absolutos (Windows e Unix)
            bool isAbsolute = (texFile.size() > 1 && texFile[1] == ':') || texFile[0] == '/';
            if (isAbsolute) {
                std::cerr << "[Mesh] Skipping absolute texture path: " << texFile << "\n";
                continue;
            }

            GLuint tid = loadTexture(dir + texFile);
            if (tid != 0)
                result[currentMat] = tid; // só sobrescreve se carregou com sucesso
        }
    }

    // Para materiais sem map_Kd válido, cria textura 1x1 com a cor Kd
    for (auto& [name, tid] : result) {
        if (tid == 0 && matKd.count(name)) {
            glm::vec3 c = matKd[name];
            unsigned char px[3] = {
                (unsigned char)(c.r * 255),
                (unsigned char)(c.g * 255),
                (unsigned char)(c.b * 255)
            };
            glGenTextures(1, &tid);
            glBindTexture(GL_TEXTURE_2D, tid);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, px);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            std::cout << "[Mesh] Created 1x1 texture for material '" << name
                << "' Kd=(" << c.r << "," << c.g << "," << c.b << ")\n";
        }
    }

    return result;
}

// ─── Vertex ───────────────────────────────────────────────────────────────────

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
    bool operator<(const Vertex& o) const {
        return memcmp(this, &o, sizeof(Vertex)) < 0;
    }
};

// ─── Mesh::Make ───────────────────────────────────────────────────────────────

MeshPtr Mesh::Make(const std::string& filename) { return MeshPtr(new Mesh(filename)); }
MeshPtr Mesh::Make() { return MeshPtr(new Mesh()); }

// ─── Construtor OBJ ───────────────────────────────────────────────────────────

Mesh::Mesh(const std::string& filename)
{
    std::vector<glm::vec3> temp_positions;
    std::vector<glm::vec3> temp_normals;
    std::vector<glm::vec2> temp_uvs;

    std::map<Vertex, unsigned int> vertexToIndex;
    std::vector<Vertex>            finalVertices;

    // Por grupo de material: índices acumulados
    struct MatGroup {
        std::string matName;
        std::vector<unsigned int> indices;
    };

    std::vector<MatGroup> groups;
    groups.push_back({ "", {} });   // grupo default (sem usemtl)

    // Mapa de nome -> textureID, preenchido se encontrarmos mtllib
    std::map<std::string, GLuint> matTextures;
    std::string dir = dirOf(filename);

    std::ifstream fp(filename);
    if (!fp) { std::cerr << "Could not open OBJ: " << filename << "\n"; exit(1); }

    std::string line;
    while (std::getline(fp, line)) {
        std::stringstream ss(line);
        std::string type;
        ss >> type;

        if (type == "mtllib") {
            std::string mtlFile;
            ss >> mtlFile;
            matTextures = parseMTL(dir + mtlFile);
        }
        else if (type == "usemtl") {
            std::string matName;
            ss >> matName;
            // Novo grupo só se o atual já tiver faces
            if (!groups.back().indices.empty())
                groups.push_back({ matName, {} });
            else
                groups.back().matName = matName;
        }
        else if (type == "v") {
            glm::vec3 v; ss >> v.x >> v.y >> v.z;
            temp_positions.push_back(v);
        }
        else if (type == "vt") {
            glm::vec2 uv; ss >> uv.x >> uv.y;
            temp_uvs.push_back(uv);
        }
        else if (type == "vn") {
            glm::vec3 n; ss >> n.x >> n.y >> n.z;
            temp_normals.push_back(n);
        }
        else if (type == "f") {
            std::vector<std::string> tokens;
            std::string tok;
            while (ss >> tok) tokens.push_back(tok);
            if (tokens.size() < 3) continue;

            for (int k = 1; k < (int)tokens.size() - 1; k++) {
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
                    groups.back().indices.push_back(vertexToIndex[vert]);
                }
            }
        }
    }
    fp.close();

    // ─── Geração de normais flat se ausentes ──────────────────────────────────
    if (temp_normals.empty()) {
        std::cout << "[OBJ loader] No normals found — generating flat normals.\n";
        std::vector<glm::vec3> genNormals(finalVertices.size(), glm::vec3(0));
        for (auto& g : groups) {
            for (int i = 0; i < (int)g.indices.size(); i += 3) {
                unsigned int i0 = g.indices[i], i1 = g.indices[i + 1], i2 = g.indices[i + 2];
                glm::vec3 a = finalVertices[i1].pos - finalVertices[i0].pos;
                glm::vec3 b = finalVertices[i2].pos - finalVertices[i0].pos;
                glm::vec3 n = glm::normalize(glm::cross(a, b));
                genNormals[i0] += n; genNormals[i1] += n; genNormals[i2] += n;
            }
        }
        for (int i = 0; i < (int)finalVertices.size(); i++)
            finalVertices[i].normal = glm::normalize(genNormals[i]);
    }

    // ─── Buffers ──────────────────────────────────────────────────────────────
    std::vector<float> coords, normals, texcoords;
    for (auto& v : finalVertices) {
        coords.push_back(v.pos.x); coords.push_back(v.pos.y); coords.push_back(v.pos.z);
        normals.push_back(v.normal.x); normals.push_back(v.normal.y); normals.push_back(v.normal.z);
        texcoords.push_back(v.uv.x); texcoords.push_back(v.uv.y);
    }

    // Index buffer unificado: concatena todos os grupos em sequência
    std::vector<unsigned int> finalIndices;
    for (auto& g : groups) {
        if (g.indices.empty()) continue;

        SubMesh sm;
        sm.textureID = matTextures.count(g.matName) ? matTextures[g.matName] : 0;
        sm.indexOffset = (unsigned int)finalIndices.size();
        sm.indexCount = (unsigned int)g.indices.size();
        m_submeshes.push_back(sm);

        finalIndices.insert(finalIndices.end(), g.indices.begin(), g.indices.end());
    }

    m_nind = (unsigned int)finalIndices.size();

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    if (!coords.empty())    SetCoordBuffer((int)coords.size(), coords.data(), 3, 0);
    if (!normals.empty())   SetNormalBuffer((int)normals.size(), normals.data(), 3, 0);
    if (!texcoords.empty()) SetTexCoordBuffer((int)texcoords.size(), texcoords.data(), 2, 0);
    SetIndexBuffer((int)finalIndices.size(), finalIndices.data());
}

Mesh::Mesh() { glGenVertexArrays(1, &m_vao); }
Mesh::~Mesh() {}

// ─── SetFallbackTexture ───────────────────────────────────────────────────────
// Chamado pelo main quando carrega textura manualmente (sem MTL)

void Mesh::SetFallbackTexture(unsigned int textureID)
{
    if (m_submeshes.empty()) return;
    // Aplica a textura em todos os grupos que não têm textura própria
    for (auto& sm : m_submeshes)
        if (sm.textureID == 0)
            sm.textureID = textureID;
}

// ─── Draw ─────────────────────────────────────────────────────────────────────

void Mesh::Draw()
{
    glBindVertexArray(m_vao);

    for (auto& sm : m_submeshes) {
        if (sm.indexCount == 0) continue;

        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, sm.textureID);

        glDrawElements(
            GL_TRIANGLES,
            sm.indexCount,
            GL_UNSIGNED_INT,
            (void*)(sm.indexOffset * sizeof(unsigned int))
        );
    }
}

// ─── Buffer helpers ───────────────────────────────────────────────────────────

void Mesh::SetCoordBuffer(int size, const float* data, int ncomp, int stride) {
    glBindVertexArray(m_vao);
    GLuint id; glGenBuffers(1, &id);
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferData(GL_ARRAY_BUFFER, size * sizeof(float), data, GL_STATIC_DRAW);
    glVertexAttribPointer(0, ncomp, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
}

void Mesh::SetNormalBuffer(int size, const float* data, int ncomp, int stride) {
    glBindVertexArray(m_vao);
    GLuint id; glGenBuffers(1, &id);
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferData(GL_ARRAY_BUFFER, size * sizeof(float), data, GL_STATIC_DRAW);
    glVertexAttribPointer(1, ncomp, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(1);
}

void Mesh::SetTangentBuffer(int size, const float* data, int ncomp, int stride) {
    glBindVertexArray(m_vao);
    GLuint id; glGenBuffers(1, &id);
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferData(GL_ARRAY_BUFFER, size * sizeof(float), data, GL_STATIC_DRAW);
    glVertexAttribPointer(2, ncomp, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(2);
}

void Mesh::SetTexCoordBuffer(int size, const float* data, int ncomp, int stride) {
    glBindVertexArray(m_vao);
    GLuint id; glGenBuffers(1, &id);
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferData(GL_ARRAY_BUFFER, size * sizeof(float), data, GL_STATIC_DRAW);
    glVertexAttribPointer(3, ncomp, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(3);
}

void Mesh::SetIndexBuffer(int size, const unsigned int* data) {
    glBindVertexArray(m_vao);
    GLuint id; glGenBuffers(1, &id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size * sizeof(unsigned int), data, GL_STATIC_DRAW);
    m_nind = size;
}