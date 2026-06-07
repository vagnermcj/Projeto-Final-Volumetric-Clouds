#include "PresetManager.h"
#include <cstdio>

// Preset Default (hardcoded)
CloudPreset PresetManager::getDefaultPreset() {
        CloudPreset def;
        def.name = "Default";

        // Valores padrão do seu código
        def.windDirection = glm::vec3(1.0f, 0.0f, 1.0f);
        def.windSpeed = 0.01f;

        def.planetRadius = 6000.0f;
        def.atmosphereStart = 100.0f;
        def.atmosphereHeight = 100.0f;
        def.atmosphereMaxDepth = 200.0f;

        def.weatherScale = 500.0f;
        def.maxCloudHeight = 25.0f;
        def.maxCloudAltitude = 80.0f;
        def.shapeScale = 200.0f;
        def.detailScale = 50.0f;
        def.detailNoiseWeight = 0.15f;
        def.shapeNoiseWeights = glm::vec4(1.0f, 0.625f, 0.25f, 0.125f);

        def.cloudMaxSteps = 128;

        def.lightDirection = glm::vec3(-0.08f, 0.35f, 1.0f);
        def.lightColor = glm::vec3(1.0f);
        def.phaseG = 0.95f;
		def.scatteringCoefficient = 0.01f;
		def.extinctionCoefficient = 0.01f;
        def.ambientIntensity = 1.3f;
        def.ambientColor = glm::vec3(1.0f);
        def.cloudTopType = 0.6f;
        def.cloudBottomType = 0.5f;
        def.silver_intensity = 0.5f;
        def.silver_spread = 2.0f;
        def.precipitation = 1.0f;
        def.lightMaxSteps = 3;
        def.enableDetailErosion = true;
        def.enableLightMarching = true;
        def.enableBeersLaw = true;
        def.enablePowderEffect = true;
        def.enablePhaseFunction = true;
        def.enableSilverSheen = true;

        def.shapeOctaves = glm::ivec3(2, 4, 8);
        def.detailOctaves = glm::ivec3(8, 16, 32);
        def.perlinScale = 1.0f;
        def.invertWorleyShape = true;
        def.invertWorleyDetail = true;

        def.coverageScale = 3.0f;
        def.heightScale = 1.5f;
        def.altitudeScale = 1.0f;
        def.skyboxName = "default";
        def.cameraPosition = glm::vec3(0.0f, 0.0f, 5.0f);
        def.cameraOrientation = glm::vec3(0.0f, 0.0f, -1.0f);

        return def;
    }

// Parse uma linha "chave=valor"
void PresetManager::parseLine(const std::string& line, CloudPreset& preset) {
        size_t pos = line.find('=');
        if (pos == std::string::npos) return;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        // Parse vec3
        auto parseVec3 = [](const std::string& s) -> glm::vec3 {
            glm::vec3 v;
            sscanf_s(s.c_str(), "%f,%f,%f", &v.x, &v.y, &v.z);
            return v;
            };

        // Parse ivec3
        auto parseIVec3 = [](const std::string& s) -> glm::ivec3 {
            glm::ivec3 v;
            sscanf_s(s.c_str(), "%d,%d,%d", &v.x, &v.y, &v.z);
            return v;
            };

        // Parse vec4
        auto parseVec4 = [](const std::string& s) -> glm::vec4 {
            glm::vec4 v;
            sscanf_s(s.c_str(), "%f,%f,%f,%f", &v.x, &v.y, &v.z, &v.w);
            return v;
            };

        // Parse bool
        auto parseBool = [](const std::string& s) -> bool {
            return s == "1" || s == "true";
            };

        // Vento
        if (key == "windDirection") preset.windDirection = parseVec3(value);
        else if (key == "windSpeed") preset.windSpeed = std::stof(value);

        // Atmosfera
        else if (key == "planetRadius") preset.planetRadius = std::stof(value);
        else if (key == "atmosphereStart") preset.atmosphereStart = std::stof(value);
        else if (key == "atmosphereHeight") preset.atmosphereHeight = std::stof(value);
        else if (key == "atmosphereMaxDepth") preset.atmosphereMaxDepth = std::stof(value);

        // Densidade
        else if (key == "weatherScale") preset.weatherScale = std::stof(value);
        else if (key == "maxCloudHeight") preset.maxCloudHeight = std::stof(value);
        else if (key == "maxCloudAltitude") preset.maxCloudAltitude = std::stof(value);
        else if (key == "shapeScale") preset.shapeScale = std::stof(value);
        else if (key == "detailScale") preset.detailScale = std::stof(value);
        else if (key == "detailNoiseWeight") preset.detailNoiseWeight = std::stof(value);
        else if (key == "shapeNoiseWeights") preset.shapeNoiseWeights = parseVec4(value);

        // Ray Marching
        else if (key == "cloudMaxSteps") preset.cloudMaxSteps = std::stoi(value);

        // Iluminação
        else if (key == "lightDirection") preset.lightDirection = parseVec3(value);
        else if (key == "lightColor") preset.lightColor = parseVec3(value);
        else if (key == "phaseG") preset.phaseG = std::stof(value);
        else if (key == "scatteringCoefficient") preset.scatteringCoefficient = std::stof(value);
        else if (key == "extinctionCoefficient") preset.extinctionCoefficient = std::stof(value);
        else if (key == "ambientIntensity") preset.ambientIntensity = std::stof(value);
        else if (key == "precipitation") preset.precipitation = std::stof(value);
        else if (key == "lightMaxSteps") preset.lightMaxSteps = std::stoi(value);

        // Noise Generation
        else if (key == "shapeOctaves") preset.shapeOctaves = parseIVec3(value);
        else if (key == "detailOctaves") preset.detailOctaves = parseIVec3(value);
        else if (key == "perlinScale") preset.perlinScale = std::stof(value);
        else if (key == "invertWorleyShape") preset.invertWorleyShape = parseBool(value);
        else if (key == "invertWorleyDetail") preset.invertWorleyDetail = parseBool(value);

        // Weather Map
        else if (key == "coverageScale") preset.coverageScale = std::stof(value);
        else if (key == "heightScale") preset.heightScale = std::stof(value);
        else if (key == "altitudeScale") preset.altitudeScale = std::stof(value);
    else if (key == "skyboxName") preset.skyboxName = value;
        else if (key == "terrainPath") preset.terrainPath = value;
        else if (key == "ambientColor") preset.ambientColor = parseVec3(value);
        else if (key == "cameraPosition") preset.cameraPosition = parseVec3(value);
        else if (key == "cameraOrientation") preset.cameraOrientation = parseVec3(value);
        else if (key == "cloudTopType") preset.cloudTopType = std::stof(value);
        else if (key == "cloudBottomType") preset.cloudBottomType = std::stof(value);
        else if (key == "silver_intensity") preset.silver_intensity = std::stof(value);
        else if (key == "silver_spread") preset.silver_spread = std::stof(value);
    else if (key == "enableDetailErosion") preset.enableDetailErosion = parseBool(value);
    else if (key == "enableLightMarching") preset.enableLightMarching = parseBool(value);
    else if (key == "enableBeersLaw") preset.enableBeersLaw = parseBool(value);
    else if (key == "enablePowderEffect") preset.enablePowderEffect = parseBool(value);
    else if (key == "enablePhaseFunction") preset.enablePhaseFunction = parseBool(value);
    else if (key == "enableSilverSheen") preset.enableSilverSheen = parseBool(value);
}

// Carrega um preset de um arquivo
CloudPreset PresetManager::loadPresetFromFile(const std::string& filepath) {
    CloudPreset preset;
    preset.name = std::filesystem::path(filepath).stem().string();

    std::ifstream file(filepath);
    std::string line;

    while (std::getline(file, line)) {
        if (!line.empty() && line[0] != '#') { // Ignora linhas vazias e comentários
            parseLine(line, preset);
        }
    }

    return preset;
}

// Salva um preset em arquivo
void PresetManager::savePresetToFile(const CloudPreset& preset, const std::string& filepath) {
    std::ofstream file(filepath);

    file << "# " << preset.name << " Preset\n\n";

    file << "# Wind\n";
    file << "windDirection=" << preset.windDirection.x << "," << preset.windDirection.y << "," << preset.windDirection.z << "\n";
    file << "windSpeed=" << preset.windSpeed << "\n\n";

    file << "# Atmosphere\n";
    file << "planetRadius=" << preset.planetRadius << "\n";
    file << "atmosphereStart=" << preset.atmosphereStart << "\n";
    file << "atmosphereHeight=" << preset.atmosphereHeight << "\n";
    file << "atmosphereMaxDepth=" << preset.atmosphereMaxDepth << "\n\n";

    file << "# Density\n";
    file << "weatherScale=" << preset.weatherScale << "\n";
    file << "maxCloudHeight=" << preset.maxCloudHeight << "\n";
    file << "maxCloudAltitude=" << preset.maxCloudAltitude << "\n";
    file << "shapeScale=" << preset.shapeScale << "\n";
    file << "detailScale=" << preset.detailScale << "\n";
    file << "detailNoiseWeight=" << preset.detailNoiseWeight << "\n";
    file << "shapeNoiseWeights=" << preset.shapeNoiseWeights.x << "," << preset.shapeNoiseWeights.y << ","
         << preset.shapeNoiseWeights.z << "," << preset.shapeNoiseWeights.w << "\n\n";

    file << "# Ray Marching\n";
    file << "cloudMaxSteps=" << preset.cloudMaxSteps << "\n\n";

    file << "# Lighting\n";
    file << "lightDirection=" << preset.lightDirection.x << "," << preset.lightDirection.y << "," << preset.lightDirection.z << "\n";
    file << "lightColor=" << preset.lightColor.x << "," << preset.lightColor.y << "," << preset.lightColor.z << "\n";
    file << "phaseG=" << preset.phaseG << "\n";
	file << "scatteringCoefficient=" << preset.scatteringCoefficient << "\n";
	file << "extinctionCoefficient=" << preset.extinctionCoefficient << "\n";
    file << "ambientIntensity=" << preset.ambientIntensity << "\n";
    file << "precipitation=" << preset.precipitation << "\n";
    file << "lightMaxSteps=" << preset.lightMaxSteps << "\n\n";

    file << "# Noise Generation\n";
    file << "shapeOctaves=" << preset.shapeOctaves.x << "," << preset.shapeOctaves.y << "," << preset.shapeOctaves.z << "\n";
    file << "detailOctaves=" << preset.detailOctaves.x << "," << preset.detailOctaves.y << "," << preset.detailOctaves.z << "\n";
    file << "perlinScale=" << preset.perlinScale << "\n";
    file << "invertWorleyShape=" << (preset.invertWorleyShape ? "1" : "0") << "\n";
    file << "invertWorleyDetail=" << (preset.invertWorleyDetail ? "1" : "0") << "\n\n";

    file << "# Weather Map\n";
    file << "coverageScale=" << preset.coverageScale << "\n";
    file << "heightScale=" << preset.heightScale << "\n";
    file << "altitudeScale=" << preset.altitudeScale << "\n";

    file << "# Terrain\n";
    file << "terrainPath=" << preset.terrainPath << "\n";

    file << "# Camera\n";
    file << "cameraPosition=" << preset.cameraPosition.x << "," << preset.cameraPosition.y << "," << preset.cameraPosition.z << "\n";
    file << "cameraOrientation=" << preset.cameraOrientation.x << "," << preset.cameraOrientation.y << "," << preset.cameraOrientation.z << "\n";

    file << "# Ambient/Visual\n";
    file << "ambientColor=" << preset.ambientColor.x << "," << preset.ambientColor.y << "," << preset.ambientColor.z << "\n";
    file << "cloudTopType=" << preset.cloudTopType << "\n";
    file << "cloudBottomType=" << preset.cloudBottomType << "\n";
    file << "silver_intensity=" << preset.silver_intensity << "\n";
    file << "silver_spread=" << preset.silver_spread << "\n";
    file << "enableDetailErosion=" << (preset.enableDetailErosion ? "1" : "0") << "\n";
    file << "enableLightMarching=" << (preset.enableLightMarching ? "1" : "0") << "\n";
    file << "enableBeersLaw=" << (preset.enableBeersLaw ? "1" : "0") << "\n";
    file << "enablePowderEffect=" << (preset.enablePowderEffect ? "1" : "0") << "\n";
    file << "enablePhaseFunction=" << (preset.enablePhaseFunction ? "1" : "0") << "\n";
    file << "enableSilverSheen=" << (preset.enableSilverSheen ? "1" : "0") << "\n";

    file << "# Skybox\n";
    file << "skyboxName=" << preset.skyboxName << "\n";

    file.close();
}

PresetManager::PresetManager() {
    // Cria pasta de presets se não existir
    std::filesystem::create_directory(presetsFolder);

    // Carrega presets da pasta
    ScanPresetsFolder();
}

// Scanneia a pasta de presets e carrega todos
void PresetManager::ScanPresetsFolder() {
    presets.clear();

    // Sempre adiciona Default primeiro
    presets.push_back(getDefaultPreset());

    // Carrega presets salvos
    if (std::filesystem::exists(presetsFolder)) {
        for (const auto& entry : std::filesystem::directory_iterator(presetsFolder)) {
            if (entry.path().extension() == ".preset") {
                CloudPreset p = loadPresetFromFile(entry.path().string());
                presets.push_back(p);
            }
        }
    }
}

    // Captura estado atual e salva como preset
void PresetManager::saveCurrentState(const std::string& name,const glm::vec3& windDir, float windSpd,
        float pRadius, float atmStart, float atmHeight, float atmDepth,
        float wScale, float shpScale, float dtlScale,
        float dtlWeight, const glm::vec4& shpWeights,
        int cloudSteps,
        const glm::vec3& lightDir, const glm::vec3& lightCol, float phase,
        float scatCoef, float extCoef,
        float ambInt, float precip, int lightSteps,
        const glm::ivec3& shpOct, const glm::ivec3& dtlOct, float pScale,
        bool invWorleyShp, bool invWorleyDtl,
        float covScale, float hScale, float altScale, const glm::vec3& cameraPos, const glm::vec3& cameraOri, const std::string& skyboxName, const std::string& terrainPath,
        const glm::vec3& ambientColor, float cloudTopType, float cloudBottomType, float silver_intensity, float silver_spread,
        bool enableDetailErosion, bool enableLightMarching, bool enableBeersLaw, bool enablePowderEffect, bool enablePhaseFunction, bool enableSilverSheen)
{
        // Initialize with defaults so performance toggles and other non-provided
        // fields keep safe default values when caller doesn't supply them.
        CloudPreset preset = getDefaultPreset();
        preset.name = name;
        preset.windDirection = windDir;
        preset.windSpeed = windSpd;
        preset.planetRadius = pRadius;
        preset.atmosphereStart = atmStart;
        preset.atmosphereHeight = atmHeight;
        preset.atmosphereMaxDepth = atmDepth;
        preset.weatherScale = wScale;
        preset.shapeScale = shpScale;
        preset.detailScale = dtlScale;
        preset.detailNoiseWeight = dtlWeight;
        preset.shapeNoiseWeights = shpWeights;
        preset.cloudMaxSteps = cloudSteps;
        preset.lightDirection = lightDir;
        preset.lightColor = lightCol;
        preset.phaseG = phase;
		preset.scatteringCoefficient = scatCoef;
		preset.extinctionCoefficient = extCoef;
        preset.precipitation = precip;
        preset.lightMaxSteps = lightSteps;
        preset.ambientColor = ambientColor;
        preset.cloudTopType = cloudTopType;
        preset.cloudBottomType = cloudBottomType;
        preset.silver_intensity = silver_intensity;
        preset.silver_spread = silver_spread;
        preset.shapeOctaves = shpOct;
        preset.detailOctaves = dtlOct;
        preset.perlinScale = pScale;
        preset.invertWorleyShape = invWorleyShp;
        preset.invertWorleyDetail = invWorleyDtl;
        preset.coverageScale = covScale;
        preset.heightScale = hScale;
        preset.altitudeScale = altScale;
        preset.cameraPosition = cameraPos;
        preset.cameraOrientation = cameraOri;
        preset.skyboxName = skyboxName;
        preset.terrainPath = terrainPath;
        // Performance toggles set from caller
        preset.enableDetailErosion = enableDetailErosion;
        preset.enableLightMarching = enableLightMarching;
        preset.enableBeersLaw = enableBeersLaw;
        preset.enablePowderEffect = enablePowderEffect;
        preset.enablePhaseFunction = enablePhaseFunction;
        preset.enableSilverSheen = enableSilverSheen;

    // Salva em arquivo
    std::string filename = presetsFolder + "/" + name + ".preset";
    savePresetToFile(preset, filename);

    // Rescanneia pasta para atualizar lista
    ScanPresetsFolder();
}

// Aplica um preset (copia valores para as variáveis)
void PresetManager::applyPreset(int index, glm::vec3& windDir, float& windSpd,
    float& pRadius, float& atmStart, float& atmHeight, float& atmDepth,
    float& wScale, float& shpScale, float& dtlScale,
    float& dtlWeight, glm::vec4& shpWeights, int& cloudSteps,
    glm::vec3& lightDir, glm::vec3& lightCol, float& phase,
    float& scattCoef, float& extCoef,
    float& ambInt, float& precip, int& lightSteps,
    glm::ivec3& shpOct, glm::ivec3& dtlOct, float& pScale,
    bool& invWorleyShp, bool& invWorleyDtl,
    float& covScale, float& hScale, float& altScale, glm::vec3& cameraPos, glm::vec3& cameraOri,
    std::string& skyboxName, std::string& terrainPath,
    glm::vec3& ambientColor, float& cloudTopType, float& cloudBottomType,
    float& silver_intensity, float& silver_spread,
    bool& enableDetailErosion, bool& enableLightMarching, bool& enableBeersLaw,
    bool& enablePowderEffect, bool& enablePhaseFunction, bool& enableSilverSheen)
{
    if (index < 0 || index >= presets.size()) return;

    const CloudPreset& p = presets[index];

    windDir = p.windDirection;
    windSpd = p.windSpeed;
    pRadius = p.planetRadius;
    atmStart = p.atmosphereStart;
    atmHeight = p.atmosphereHeight;
    atmDepth = p.atmosphereMaxDepth;
    wScale = p.weatherScale;
    shpScale = p.shapeScale;
    dtlScale = p.detailScale;
    dtlWeight = p.detailNoiseWeight;
    shpWeights = p.shapeNoiseWeights;
    cloudSteps = p.cloudMaxSteps;
    lightDir = p.lightDirection;
    lightCol = p.lightColor;
    phase = p.phaseG;
	scattCoef = p.scatteringCoefficient;
	extCoef = p.extinctionCoefficient;
    ambInt = p.ambientIntensity;
    precip = p.precipitation;
    lightSteps = p.lightMaxSteps;
    shpOct = p.shapeOctaves;
    dtlOct = p.detailOctaves;
    pScale = p.perlinScale;
    invWorleyShp = p.invertWorleyShape;
    invWorleyDtl = p.invertWorleyDetail;
    covScale = p.coverageScale;
    hScale = p.heightScale;
    altScale = p.altitudeScale;
    // camera
    cameraPos = p.cameraPosition;
    cameraOri = p.cameraOrientation;
    skyboxName = p.skyboxName;
    terrainPath = p.terrainPath;
    ambientColor = p.ambientColor;
    cloudTopType = p.cloudTopType;
    cloudBottomType = p.cloudBottomType;
    silver_intensity = p.silver_intensity;
    silver_spread = p.silver_spread;
    // performance toggles
    enableDetailErosion = p.enableDetailErosion;
    enableLightMarching = p.enableLightMarching;
    enableBeersLaw = p.enableBeersLaw;
    enablePowderEffect = p.enablePowderEffect;
    enablePhaseFunction = p.enablePhaseFunction;
    enableSilverSheen = p.enableSilverSheen;
}

// Deleta um preset (não pode deletar Default)
void PresetManager::deletePreset(int index) {
    if (index <= 0 || index >= presets.size()) return; // Não pode deletar Default (index 0)

    std::string filename = presetsFolder + "/" + presets[index].name + ".preset";
    std::filesystem::remove(filename);
    ScanPresetsFolder();
}

// Retorna nomes dos presets para ImGui::Combo
std::vector<const char*> PresetManager::getPresetNames() {
    std::vector<const char*> names;
    for (auto& p : presets) {
        names.push_back(p.name.c_str());
    }
    return names;
}

int PresetManager::getPresetCount() {
    return presets.size();
}

CloudPreset PresetManager::getPreset(int index) {
    if (index < 0 || index >= static_cast<int>(presets.size())) return getDefaultPreset();
    // no-op: ensure function ends properly while keeping behavior unchanged
    return presets[index];
}
