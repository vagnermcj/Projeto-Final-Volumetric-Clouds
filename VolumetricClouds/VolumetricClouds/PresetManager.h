#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>

struct CloudPreset {
    std::string name;

    // Vento
    glm::vec3 windDirection;
    float windSpeed;

    // Atmosfera
    float planetRadius;
    float atmosphereStart;
    float atmosphereHeight;
    float atmosphereMaxDepth;

    // Densidade
    float weatherScale;
    float maxCloudHeight;
    float maxCloudAltitude;
    float shapeScale;
    float detailScale;
    float detailNoiseWeight;
    glm::vec4 shapeNoiseWeights;

    // Ray Marching
    int cloudMaxSteps;

    // Iluminação
    glm::vec3 lightDirection;
    glm::vec3 lightColor;
    float phaseG;
    float scatteringCoefficient;
	float extinctionCoefficient;
    float ambientIntensity;
    glm::vec3 ambientColor;
    float precipitation;
    int lightMaxSteps;

    // Extra visual
    float cloudTopType;
    float cloudBottomType;
    float silver_intensity;
    float silver_spread;

    // Noise Generation
    glm::ivec3 shapeOctaves;
    glm::ivec3 detailOctaves;
    float perlinScale;
    bool invertWorleyShape;
    bool invertWorleyDetail;

    // Weather Map
    float coverageScale;
    float heightScale;
    float altitudeScale;

    // Camera
    glm::vec3 cameraPosition;
    glm::vec3 cameraOrientation;

    // Skybox
    std::string skyboxName;
    bool useSkybox = true;
    glm::vec3 gradientColorTop = glm::vec3(0.1f, 0.3f, 0.6f);
    glm::vec3 gradientColorBottom = glm::vec3(0.6f, 0.75f, 0.9f);
    // Terrain (path or name)
    std::string terrainPath;
    glm::vec3 terrainPosition = glm::vec3(0.0f);
    glm::vec3 terrainScale = glm::vec3(1.0f);
    // Performance toggles
    bool enableDetailErosion;
    bool enableLightMarching;
    bool enableBeersLaw;
    bool enablePowderEffect;
    bool enablePhaseFunction;
    bool enableSilverSheen;
};

class PresetManager {
private:
    std::vector<CloudPreset> presets;
    std::string presetsFolder = "presets";
	void parseLine(const std::string& line, CloudPreset& preset);
	CloudPreset getDefaultPreset();
	CloudPreset loadPresetFromFile(const std::string& filepath);
	void savePresetToFile(const CloudPreset& preset, const std::string& filepath);

public:
	PresetManager();
	void ScanPresetsFolder();
    CloudPreset getPreset(int index);
    void saveCurrentState(const std::string& name, const glm::vec3& windDir, float windSpd,
        float pRadius, float atmStart, float atmHeight, float atmDepth,
        float wScale, float shpScale, float dtlScale,
        float dtlWeight, const glm::vec4& shpWeights,
        int cloudSteps,
        const glm::vec3& lightDir, const glm::vec3& lightCol, float phase,
		float scattCoef, float extCoef,
        float ambInt, float precip, int lightSteps,
        const glm::ivec3& shpOct, const glm::ivec3& dtlOct, float pScale,
        bool invWorleyShp, bool invWorleyDtl,
        float covScale, float hScale, float altScale, const glm::vec3& cameraPos, const glm::vec3& cameraOri, const std::string& skyboxName, const std::string& terrainPath, const glm::vec3& terrainPos, const glm::vec3& terrainScale,
        bool useSkybox, const glm::vec3& gradientTop, const glm::vec3& gradientBottom, float ambIntensity,
        const glm::vec3& ambientColor, float cloudTopType, float cloudBottomType, float silver_intensity, float silver_spread,
        bool enableDetailErosion, bool enableLightMarching, bool enableBeersLaw, bool enablePowderEffect, bool enablePhaseFunction, bool enableSilverSheen);
    void applyPreset(int index, glm::vec3& windDir, float& windSpd,
        float& pRadius, float& atmStart, float& atmHeight, float& atmDepth,
        float& wScale, float& shpScale, float& dtlScale,
        float& dtlWeight, glm::vec4& shpWeights, int& cloudSteps,
        glm::vec3& lightDir, glm::vec3& lightCol, float& phase,
        float& scattCoef, float& extCoef,
        float& ambInt, float& precip, int& lightSteps,
        glm::ivec3& shpOct, glm::ivec3& dtlOct, float& pScale,
        bool& invWorleyShp, bool& invWorleyDtl,
        float& covScale, float& hScale, float& altScale, glm::vec3& cameraPos, glm::vec3& cameraOri,
        std::string& skyboxName, std::string& terrainPath, glm::vec3& terrainPos, glm::vec3& terrainScale, bool& useSkybox, glm::vec3& gradientTop, glm::vec3& gradientBottom, float& ambIntensity,
        glm::vec3& ambientColor, float& cloudTopType, float& cloudBottomType,
        float& silver_intensity, float& silver_spread,
        bool& enableDetailErosion, bool& enableLightMarching, bool& enableBeersLaw,
        bool& enablePowderEffect, bool& enablePhaseFunction, bool& enableSilverSheen);
	void deletePreset(int index);
	std::vector<const char*> getPresetNames();
	int getPresetCount();
};