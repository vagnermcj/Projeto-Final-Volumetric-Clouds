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
        def.scatteringColor = glm::vec3(0.8f, 0.9f, 1.0f);
        def.absorptionColor = glm::vec3(0.05f, 0.05f, 0.05f);
        def.ambientColor = glm::vec3(1.0f, 1.0f, 1.0f);
        def.ambientIntensity = 1.3f;
        def.precipitation = 1.0f;
        def.lightMaxSteps = 3;

        def.shapeOctaves = glm::ivec3(2, 4, 8);
        def.detailOctaves = glm::ivec3(8, 16, 32);
        def.perlinScale = 1.0f;
        def.invertWorleyShape = true;
        def.invertWorleyDetail = true;

        def.coverageScale = 3.0f;
        def.heightScale = 1.5f;
        def.altitudeScale = 1.0f;
        def.coverageMin = 0.4f;
        def.coverageMax = 0.7f;
        def.altitudePointCount = 3;
        def.altitudeBlobMinRadius = 0.05f;
        def.altitudeBlobMaxRadius = 0.15f;
        def.skyboxName = "default";

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
        else if (key == "scatteringColor") preset.scatteringColor = parseVec3(value);
        else if (key == "absorptionColor") preset.absorptionColor = parseVec3(value);
        else if (key == "ambientColor") preset.ambientColor = parseVec3(value);
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
        else if (key == "coverageMin") preset.coverageMin = std::stof(value);
        else if (key == "coverageMax") preset.coverageMax = std::stof(value);
        else if (key == "altitudePointCount") preset.altitudePointCount = std::stoi(value);
        else if (key == "altitudeBlobMinRadius") preset.altitudeBlobMinRadius = std::stof(value);
        else if (key == "altitudeBlobMaxRadius") preset.altitudeBlobMaxRadius = std::stof(value);
        else if (key == "skyboxName") preset.skyboxName = value;
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
    file << "scatteringColor=" << preset.scatteringColor.x << "," << preset.scatteringColor.y << "," << preset.scatteringColor.z << "\n";
    file << "absorptionColor=" << preset.absorptionColor.x << "," << preset.absorptionColor.y << "," << preset.absorptionColor.z << "\n";
    file << "ambientColor=" << preset.ambientColor.x << "," << preset.ambientColor.y << "," << preset.ambientColor.z << "\n";
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
    file << "coverageMin=" << preset.coverageMin << "\n";
    file << "coverageMax=" << preset.coverageMax << "\n";
    file << "altitudePointCount=" << preset.altitudePointCount << "\n";
    file << "altitudeBlobMinRadius=" << preset.altitudeBlobMinRadius << "\n";
    file << "altitudeBlobMaxRadius=" << preset.altitudeBlobMaxRadius << "\n";

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
        float wScale, float maxHeight, float maxAlt, float shpScale, float dtlScale,
        float dtlWeight, const glm::vec4& shpWeights,
        int cloudSteps,
        const glm::vec3& lightDir, const glm::vec3& lightCol, float phase,
        const glm::vec3& scatCol, const glm::vec3& absCol, const glm::vec3& ambCol,
        float ambInt, float precip, int lightSteps,
        const glm::ivec3& shpOct, const glm::ivec3& dtlOct, float pScale,
        bool invWorleyShp, bool invWorleyDtl,
        float covScale, float hScale, float altScale, float covMin, float covMax,
        int altCount, float altMinR, float altMaxR, const std::string& skyboxName)
{
        CloudPreset preset;
        preset.name = name;
        preset.windDirection = windDir;
        preset.windSpeed = windSpd;
        preset.planetRadius = pRadius;
        preset.atmosphereStart = atmStart;
        preset.atmosphereHeight = atmHeight;
        preset.atmosphereMaxDepth = atmDepth;
        preset.weatherScale = wScale;
        preset.maxCloudHeight = maxHeight;
        preset.maxCloudAltitude = maxAlt;
        preset.shapeScale = shpScale;
        preset.detailScale = dtlScale;
        preset.detailNoiseWeight = dtlWeight;
        preset.shapeNoiseWeights = shpWeights;
        preset.cloudMaxSteps = cloudSteps;
        preset.lightDirection = lightDir;
        preset.lightColor = lightCol;
        preset.phaseG = phase;
        preset.scatteringColor = scatCol;
        preset.absorptionColor = absCol;
        preset.ambientColor = ambCol;
        preset.ambientIntensity = ambInt;
        preset.precipitation = precip;
        preset.lightMaxSteps = lightSteps;
        preset.shapeOctaves = shpOct;
        preset.detailOctaves = dtlOct;
        preset.perlinScale = pScale;
        preset.invertWorleyShape = invWorleyShp;
        preset.invertWorleyDetail = invWorleyDtl;
        preset.coverageScale = covScale;
        preset.heightScale = hScale;
        preset.altitudeScale = altScale;
        preset.coverageMin = covMin;
        preset.coverageMax = covMax;
        preset.altitudePointCount = altCount;
        preset.altitudeBlobMinRadius = altMinR;
        preset.altitudeBlobMaxRadius = altMaxR;
        preset.skyboxName = skyboxName;

    // Salva em arquivo
    std::string filename = presetsFolder + "/" + name + ".preset";
    savePresetToFile(preset, filename);

    // Rescanneia pasta para atualizar lista
    ScanPresetsFolder();
}

    // Aplica um preset (copia valores para as variáveis)
void PresetManager::applyPreset(int index,
        // Vento
        glm::vec3& windDir, float& windSpd,
        // Atmosfera
        float& pRadius, float& atmStart, float& atmHeight, float& atmDepth,
        // Densidade
        float& wScale, float& maxHeight, float& maxAlt, float& shpScale, float& dtlScale,
        float& dtlWeight, glm::vec4& shpWeights,
        // Ray Marching
        int& cloudSteps,
        // Iluminação
        glm::vec3& lightDir, glm::vec3& lightCol, float& phase,
        glm::vec3& scatCol, glm::vec3& absCol, glm::vec3& ambCol,
        float& ambInt, float& precip, int& lightSteps,
        // Noise
        glm::ivec3& shpOct, glm::ivec3& dtlOct, float& pScale,
        bool& invWorleyShp, bool& invWorleyDtl,
        // Weather
        float& covScale, float& hScale, float& altScale, float& covMin, float& covMax,
        int& altCount, float& altMinR, float& altMaxR, std::string& skyboxName)
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
    maxHeight = p.maxCloudHeight;
    maxAlt = p.maxCloudAltitude;
    shpScale = p.shapeScale;
    dtlScale = p.detailScale;
    dtlWeight = p.detailNoiseWeight;
    shpWeights = p.shapeNoiseWeights;
    cloudSteps = p.cloudMaxSteps;
    lightDir = p.lightDirection;
    lightCol = p.lightColor;
    phase = p.phaseG;
    scatCol = p.scatteringColor;
    absCol = p.absorptionColor;
    ambCol = p.ambientColor;
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
    covMin = p.coverageMin;
    covMax = p.coverageMax;
    altCount = p.altitudePointCount;
    altMinR = p.altitudeBlobMinRadius;
    altMaxR = p.altitudeBlobMaxRadius;
	skyboxName = p.skyboxName;
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
