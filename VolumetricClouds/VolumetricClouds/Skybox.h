#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <iostream>
#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

class SkyboxManager {
public:
    struct SkyboxInfo {
        std::string name;           // Nome da pasta (ex: "default", "sunset")
        std::string path;           // Caminho completo (ex: "skyboxes/default")
        unsigned int textureID;     // ID da textura OpenGL (0 se não carregado)
        bool isLoaded;              // Se o cubemap já foi carregado
    };

private:
    std::vector<SkyboxInfo> skyboxes;
    int currentSkyboxIndex = 0;
    std::string skyboxesBaseFolder = "skyboxes";

    // Detecta qual formato de nomenclatura a pasta usa
    enum NamingFormat {
        FORMAT_PX_PY_PZ,    // px, nx, py, ny, pz, nz
        FORMAT_SIDES,       // right, left, top, bottom, front, back
        FORMAT_UNKNOWN
    };

    NamingFormat detectNamingFormat(const std::string& folderPath) {
        // Checa se tem arquivos px/py/pz
        std::vector<std::string> pxFormats = { "px", "nx", "py", "ny", "pz", "nz" };
        std::vector<std::string> extensions = { ".png", ".jpg", ".jpeg", ".tga", ".bmp" };

        bool hasPxFormat = false;
        for (const auto& name : pxFormats) {
            for (const auto& ext : extensions) {
                if (std::filesystem::exists(folderPath + "/" + name + ext)) {
                    hasPxFormat = true;
                    break;
                }
            }
            if (hasPxFormat) break;
        }

        if (hasPxFormat) return FORMAT_PX_PY_PZ;

        // Checa se tem arquivos front/back/top
        std::vector<std::string> sideFormats = { "right", "left", "top", "bottom", "front", "back" };
        bool hasSideFormat = false;
        for (const auto& name : sideFormats) {
            for (const auto& ext : extensions) {
                if (std::filesystem::exists(folderPath + "/" + name + ext)) {
                    hasSideFormat = true;
                    break;
                }
            }
            if (hasSideFormat) break;
        }

        if (hasSideFormat) return FORMAT_SIDES;

        return FORMAT_UNKNOWN;
    }

    // Encontra arquivo com qualquer extensão suportada
    std::string findFileWithExtension(const std::string& basePath, const std::string& basename) {
        std::vector<std::string> extensions = { ".png", ".jpg", ".jpeg", ".tga", ".bmp" };

        for (const auto& ext : extensions) {
            std::string fullPath = basePath + "/" + basename + ext;
            if (std::filesystem::exists(fullPath)) {
                return fullPath;
            }
        }

        return ""; // Não encontrado
    }

    // Carrega cubemap de uma pasta específica
    unsigned int loadCubemapFromFolder(const std::string& folderPath) {
        NamingFormat format = detectNamingFormat(folderPath);

        if (format == FORMAT_UNKNOWN) {
            std::cout << "Skybox warning: Could not detect naming format in " << folderPath << "\n";
            return 0;
        }

        std::vector<std::string> faceFiles;

        if (format == FORMAT_PX_PY_PZ) {
            // Ordem OpenGL: +X, -X, +Y, -Y, +Z, -Z
            faceFiles = {
                findFileWithExtension(folderPath, "px"),  // Right
                findFileWithExtension(folderPath, "nx"),  // Left
                findFileWithExtension(folderPath, "py"),  // Top
                findFileWithExtension(folderPath, "ny"),  // Bottom
                findFileWithExtension(folderPath, "pz"),  // Front
                findFileWithExtension(folderPath, "nz")   // Back
            };
        }
        else if (format == FORMAT_SIDES) {
            // Ordem OpenGL: +X, -X, +Y, -Y, +Z, -Z
            faceFiles = {
                findFileWithExtension(folderPath, "right"),
                findFileWithExtension(folderPath, "left"),
                findFileWithExtension(folderPath, "top"),
                findFileWithExtension(folderPath, "bottom"),
                findFileWithExtension(folderPath, "front"),
                findFileWithExtension(folderPath, "back")
            };
        }

        // Verifica se todos os arquivos foram encontrados
        for (size_t i = 0; i < faceFiles.size(); i++) {
            if (faceFiles[i].empty()) {
                std::cout << "Skybox error: Missing face " << i << " in " << folderPath << "\n";
                return 0;
            }
        }

        // Carrega o cubemap
        unsigned int textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

        int width, height, channels;
        for (unsigned int i = 0; i < faceFiles.size(); i++) {
            unsigned char* data = stbi_load(faceFiles[i].c_str(), &width, &height, &channels, 0);
            if (data) {
                GLenum format = GL_RGB;
                if (channels == 1) format = GL_RED;
                else if (channels == 3) format = GL_RGB;
                else if (channels == 4) format = GL_RGBA;

                glTexImage2D(
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                    0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data
                );
                stbi_image_free(data);
            }
            else {
                std::cout << "Cubemap face failed to load: " << faceFiles[i] << "\n";
                stbi_image_free(data);
                glDeleteTextures(1, &textureID);
                return 0;
            }
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        return textureID;
    }

public:
    SkyboxManager() {
        // Cria pasta base se não existir
        std::filesystem::create_directory(skyboxesBaseFolder);
        scanSkyboxes();
    }

    ~SkyboxManager() {
        // Limpa texturas OpenGL
        for (auto& skybox : skyboxes) {
            if (skybox.isLoaded && skybox.textureID != 0) {
                glDeleteTextures(1, &skybox.textureID);
            }
        }
    }

    // Scanneia a pasta de skyboxes
    void scanSkyboxes() {
        skyboxes.clear();

        if (!std::filesystem::exists(skyboxesBaseFolder)) {
            std::cout << "Skybox folder not found: " << skyboxesBaseFolder << "\n";
            return;
        }

        // Itera por todas as subpastas
        for (const auto& entry : std::filesystem::directory_iterator(skyboxesBaseFolder)) {
            if (entry.is_directory()) {
                SkyboxInfo info;
                info.name = entry.path().filename().string();
                info.path = entry.path().string();
                info.textureID = 0;
                info.isLoaded = false;
                skyboxes.push_back(info);
            }
        }

        if (skyboxes.empty()) {
            std::cout << "Warning: No skyboxes found in " << skyboxesBaseFolder << "\n";
        }
    }

    // Carrega uma skybox específica (lazy loading)
    unsigned int getSkyboxTexture(int index) {
        if (index < 0 || index >= skyboxes.size()) {
            std::cout << "Invalid skybox index: " << index << "\n";
            return 0;
        }

        SkyboxInfo& skybox = skyboxes[index];

        // Se já foi carregada, retorna o ID
        if (skybox.isLoaded) {
            return skybox.textureID;
        }

        // Caso contrário, carrega agora
        skybox.textureID = loadCubemapFromFolder(skybox.path);
        skybox.isLoaded = (skybox.textureID != 0);

        if (!skybox.isLoaded) {
            std::cout << "Failed to load skybox: " << skybox.name << "\n";
        }

        return skybox.textureID;
    }

    // Carrega skybox pelo nome
    unsigned int getSkyboxTextureByName(const std::string& name) {
        for (size_t i = 0; i < skyboxes.size(); i++) {
            if (skyboxes[i].name == name) {
                return getSkyboxTexture(i);
            }
        }

        std::cout << "Skybox not found: " << name << "\n";
        return 0;
    }

    // Define skybox atual
    void setCurrentSkybox(int index) {
        if (index >= 0 && index < skyboxes.size()) {
            currentSkyboxIndex = index;
        }
    }

    // Define skybox atual pelo nome
    bool setCurrentSkyboxByName(const std::string& name) {
        for (size_t i = 0; i < skyboxes.size(); i++) {
            if (skyboxes[i].name == name) {
                currentSkyboxIndex = i;
                return true;
            }
        }
        return false;
    }

    // Retorna a skybox atual
    unsigned int getCurrentSkyboxTexture() {
        return getSkyboxTexture(currentSkyboxIndex);
    }

    // Retorna o nome da skybox atual
    std::string getCurrentSkyboxName() const {
        if (currentSkyboxIndex >= 0 && currentSkyboxIndex < skyboxes.size()) {
            return skyboxes[currentSkyboxIndex].name;
        }
        return "";
    }

    // Retorna nomes para ImGui::Combo
    std::vector<const char*> getSkyboxNames() const {
        std::vector<const char*> names;
        for (const auto& skybox : skyboxes) {
            names.push_back(skybox.name.c_str());
        }
        return names;
    }

    int getSkyboxCount() const {
        return skyboxes.size();
    }

    int getCurrentIndex() const {
        return currentSkyboxIndex;
    }

    // Força recarregar todas as skyboxes
    void reloadAll() {
        for (auto& skybox : skyboxes) {
            if (skybox.isLoaded && skybox.textureID != 0) {
                glDeleteTextures(1, &skybox.textureID);
                skybox.textureID = 0;
                skybox.isLoaded = false;
            }
        }
        scanSkyboxes();
    }
};