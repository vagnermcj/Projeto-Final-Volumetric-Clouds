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
    glm::vec3 scatteringColor;
    glm::vec3 absorptionColor;
    glm::vec3 ambientColor;
    float ambientIntensity;
    float precipitation;
    int lightMaxSteps;

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
    float coverageMin;
    float coverageMax;
    int altitudePointCount;
    float altitudeBlobMinRadius;
    float altitudeBlobMaxRadius;

    // Skybox
    std::string skyboxName;
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
	void saveCurrentState(const std::string& name, const glm::vec3& windDir, float windSpd,
        float pRadius, float atmStart, float atmHeight, float atmDepth,
        float wScale, float maxHeight, float maxAlt, float shpScale, float dtlScale,
        float dtlWeight, const glm::vec4& shpWeights,
        int cloudSteps,
        const glm::vec3& lightDir, const glm::vec3& lightCol, float phase,
        const glm::vec3& scatCol, const glm::vec3& absCol, const glm::vec3& ambCol,
        float ambInt, float precip, int lightSteps,
        const glm::ivec3& shpOct, const glm::ivec3& dtlOct, float pScale,
        bool invWorleyShp, bool invWorleyDtl,
        float covScale, float hScale, float altScale, float covMin, float covMax,
        int altCount, float altMinR, float altMaxR, const std::string& skyboxName);
	void applyPreset(int index, glm::vec3& windDir, float& windSpd,
		float& pRadius, float& atmStart, float& atmHeight, float& atmDepth,
		float& wScale, float& maxHeight, float& maxAlt, float& shpScale, float& dtlScale,
		float& dtlWeight, glm::vec4& shpWeights, int& cloudSteps,
		glm::vec3& lightDir, glm::vec3& lightCol, float& phase,
		glm::vec3& scatCol, glm::vec3& absCol, glm::vec3& ambCol,
		float& ambInt, float& precip, int& lightSteps, glm::ivec3& shpOct, glm::ivec3& dtlOct, float& pScale,
		bool& invWorleyShp, bool& invWorleyDtl, float& covScale, float& hScale, float& altScale, float& covMin, float& covMax,
		int& altCount, float& altMinR, float& altMaxR, std::string& skyboxName);
	void deletePreset(int index);
	std::vector<const char*> getPresetNames();
	int getPresetCount();
};