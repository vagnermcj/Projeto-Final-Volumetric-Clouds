#include <iostream>
#include <random>
#include <chrono>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/ImGuiFileDialog.h>


#include "mesh.h"
#include "shaderClass.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "Camera.h"
#include "WorleyNoise3D.h"
#include "PresetManager.h"
#include "Skybox.h"

const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 900;

static GLuint quadID;

GLuint shapeTexture, detailTexture, weatherTexture;
GLuint ssboShape, ssboDetail, ssboAltitude;
GLuint previewFBO, previewTex;
GLuint mainFBO, mainColorTex, mainDepthTex;
GLuint terrainTexture = 0;
int mainFBOWidth = 800, mainFBOHeight = 800;

Shader* sliceShader;
Shader* slice2DShader;

glm::ivec3 shapeOctaves(2, 4, 8);
glm::ivec3 detailOctaves(8, 12, 16);
float perlinScale = 1.0f;
bool needsUpdate = true;
bool invertWorleyShape = true;
bool invertWorleyDetail = true;

struct NoiseBufferData {
    std::vector<glm::vec3> allPoints;
    glm::ivec3 offsets;
};


// ─── Protótipos ───────────────────────────────────────────────────────────────
void         initTextures();
void         initTexturePreview();
void         initScreenQuad();
void         initMainFBO(int width, int height);
void         drawScreenQuad();
void         framebuffer_size_callback(GLFWwindow* window, int width, int height);
void         updateNoiseSSBO(GLuint ssbo, glm::ivec3 octaves, glm::ivec3& offsetsOut);
void         dispatchNoiseCompute(Shader& shader, GLuint tex, glm::ivec3 res, glm::ivec3 octaves, glm::ivec3 offsets, bool isShape, bool invertWorley);
void         dispatchWeatherCompute(Shader& shader, GLuint tex);
void         renderTexturePreview(Shader& shader, GLuint tex3D, float slice, int channel);
void         renderTexturePreview2D(Shader& shader, GLuint tex2D, int channel);
unsigned int loadCubemap(std::vector<std::string> faces);

auto ceilDiv = [](int a, int b) { return (a + b - 1) / b; };

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Clouds", NULL, NULL);
    if (!window) { std::cout << "Failed to create GLFW window\n"; glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);
    gladLoadGL();
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    MeshPtr terrainMesh = nullptr;
    Shader* terrainProgram = nullptr; // ponteiro para inicializar após o contexto GL
    glm::vec3 terrainColor(0.45f, 0.38f, 0.28f);
    glm::vec3 terrainPosition(0.0f, 0.0f, 0.0f);
    glm::vec3 terrainScale(1.0f, 1.0f, 1.0f);
    std::string currentTerrainPath = "";


    Shader rayMarchingProgram("RayMarch.vert", "RayMarch.frag");
    Shader noiseCompute("NoiseCompute.glsl");
    Shader weatherCompute("WeatherCompute.glsl");
    terrainProgram = new Shader("Terrain.vert", "Terrain.frag");
    sliceShader = new Shader("RayMarch.vert", "SlicePreview.frag");
    slice2DShader = new Shader("RayMarch.vert", "SlicePreview2D.frag");

    // ─── Recursos ─────────────────────────────────────────────────────────────
    initTextures();
    initTexturePreview();
    initScreenQuad();
    initMainFBO(SCR_WIDTH, SCR_HEIGHT);
    glGenBuffers(1, &ssboShape);
    glGenBuffers(1, &ssboDetail);



    // ─── ImGui ────────────────────────────────────────────────────────────────
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 430 core");
    Camera camera(SCR_WIDTH, SCR_HEIGHT, glm::vec3(0.0f, 0.0f, 5.0f));
    // track camera for presets
    glm::vec3 initialCameraPos = camera.Position;
    glm::vec3 initialCameraOri = camera.Orientation;
    float cameraFar = 1000.0f;

    // ─── Gerenciamento de Presets ─────────────────────────────────────────────
    PresetManager presetManager;
    int selectedPreset = 0;
    char newPresetName[64] = "";

    SkyboxManager skyboxManager;
    std::string currentSkyboxName = "default";



    // ─── Parâmetros de Iluminação ─────────────────────────────────────────────
    glm::vec3 lightDirection(-0.08f, 0.35f, 1.0f);
    glm::vec3 lightColor(1.0f);
    glm::vec3 ambientColor(1.0f);
    float silver_intensity = 0.5f;
    float silver_spread = 0.5f;
    float phaseG = 0.6f;
    float extinctionCoef = 0.9f;
    float scatteringCoef = 0.2f;
    float ambientIntensity = 1.0f;
    float precipitation = 1.0f;
    int lightMaxSteps = 3;

    // ─── Parâmetros de Vento ──────────────────────────────────────────────────
    glm::vec3 windDirection(1.0f, 0.0f, 1.0f);
    float     windSpeed = 0.01f;

    // ─── Parâmetros da Atmosfera ──────────────────────────────────────────────
    float planetRadius = 6000.0f;
    float atmosphereStart = 100.0f;
    float atmosphereHeight = 250.0f;
    float atmosphereMaxDepth = 200.0f;
    float innerCloudRadius, outerCloudRadius;

    // ─── Parâmetros de Densidade ──────────────────────────────────────────────
    float weatherScale = 500.0f;
    float erosionValue = 0.05f;
    glm::vec4 shapeNoiseWeights(1.0f, 0.625f, 0.25f, 0.125f);
    float shapeScale = 180.0f;
    float detailScale = 20.0f;
    int cloudMaxSteps = 40;
    float cloudTopType = 0.6f;
    float cloudBottomType = 0.5f;

    // ─── Performance Benchmark Controls ───────────────────────────────────────
    bool enableDetailErosion = true;
    bool enableLightMarching = true;
    bool enableBeersLaw = true;
    bool enablePowderEffect = true;
    bool enablePhaseFunction = true;
    bool enableSilverSheen = true;

    // ─── Parâmetros do Weather Map ────────────────────────────────────────────
    float coverageScale = 3.0f;
    float heightScale = 1.5f;
    float altitudeScale = 1.0f;
    glm::vec2 weatherNoiseOffset(0.0f);

    std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_real_distribution<float> dist(0.0f, 1024.0f);   // range large enough
    weatherNoiseOffset = glm::vec2(dist(rng), dist(rng));


    // Skybox
    unsigned int cubemapTexture = 0;
    bool useSkybox = true;
    glm::vec3 gradientColorTop(0.1f, 0.3f, 0.6f);
    glm::vec3 gradientColorBottom(0.6f, 0.75f, 0.9f);
    if (skyboxManager.getSkyboxCount() > 0) {
        cubemapTexture = skyboxManager.getCurrentSkyboxTexture();
        currentSkyboxName = skyboxManager.getCurrentSkyboxName();
    }
    else {
        std::cout << "Warning: No skyboxes found! Create folder: skyboxes/default/\n";
    }

    // ─── Estado do Preview ────────────────────────────────────────────────────
    static float previewSlice = 0.5f;
    static int   previewChannel = 0;
    static int   previewTarget = 0; // 0=Shape, 1=Detail, 2=Weather

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        // Compute Shaders 
        if (needsUpdate) {
            glm::ivec3 sOff, dOff;
            updateNoiseSSBO(ssboShape, shapeOctaves, sOff);
            updateNoiseSSBO(ssboDetail, detailOctaves, dOff);
            dispatchNoiseCompute(noiseCompute, shapeTexture, glm::ivec3(128, 32, 128), shapeOctaves, sOff, true, invertWorleyShape);
            dispatchNoiseCompute(noiseCompute, detailTexture, glm::ivec3(32, 32, 32), detailOctaves, dOff, false, invertWorleyDetail);

            weatherCompute.Activate();
            weatherCompute.SetUniform("coverageScale", coverageScale);
            weatherCompute.SetUniform("heightScale", heightScale);
            weatherCompute.SetUniform("altitudeScale", altitudeScale);
            weatherCompute.SetUniform("noiseOffset", weatherNoiseOffset);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboAltitude);
            dispatchWeatherCompute(weatherCompute, weatherTexture);

            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
            needsUpdate = false;
        }

        //Render Scene
        glBindFramebuffer(GL_FRAMEBUFFER, mainFBO);
        glViewport(0, 0, mainFBOWidth, mainFBOHeight);
        glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        camera.updateMatrix(45.0f, 0.1f, cameraFar);


        if (terrainMesh)
        {
            glEnable(GL_DEPTH_TEST);

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, terrainPosition);
            model = glm::scale(model, terrainScale);

            terrainProgram->Activate();
            terrainProgram->SetUniform("model", model);
            terrainProgram->SetUniform("camMatrix", camera.GetMatrix());
            terrainProgram->SetUniform("camPos", camera.Position);
            terrainProgram->SetUniform("lightDirection", lightDirection);
            terrainProgram->SetUniform("lightColor", lightColor);
            terrainProgram->SetUniform("ambientColor", ambientColor);
            terrainProgram->SetUniform("ambientIntensity", ambientIntensity);
            terrainProgram->SetUniform("terrainColor", terrainColor);
            terrainProgram->SetUniform("terrainTex", 5);
            terrainProgram->SetUniform("hasTexture", terrainTexture != 0);
            terrainMesh->Draw();

            glDisable(GL_DEPTH_TEST);
        }

        rayMarchingProgram.Activate();

        // Texturas
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_3D, shapeTexture);   rayMarchingProgram.SetUniform("shapeNoise", 0);
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_3D, detailTexture);  rayMarchingProgram.SetUniform("detailNoise", 1);
        glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, weatherTexture); rayMarchingProgram.SetUniform("weatherMap", 2);
        glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture); rayMarchingProgram.SetUniform("skybox", 3);
        rayMarchingProgram.SetUniform("useSkybox", useSkybox);
        rayMarchingProgram.SetUniform("gradientColorTop", gradientColorTop);
        rayMarchingProgram.SetUniform("gradientColorBottom", gradientColorBottom);

        // Uniforms — Atmosfera
        innerCloudRadius = planetRadius + atmosphereStart;
        outerCloudRadius = planetRadius + atmosphereStart + atmosphereHeight;
        rayMarchingProgram.SetUniform("planetRadius", planetRadius);
        rayMarchingProgram.SetUniform("innerCloudRadius", innerCloudRadius);
        rayMarchingProgram.SetUniform("outerCloudRadius", outerCloudRadius);
        rayMarchingProgram.SetUniform("atmosphereStart", atmosphereStart);
        rayMarchingProgram.SetUniform("atmosphereHeight", atmosphereHeight);
        rayMarchingProgram.SetUniform("atmosphereMaxDepth", atmosphereMaxDepth);

        // Uniforms — Densidade
        rayMarchingProgram.SetUniform("weatherScale", weatherScale);
        rayMarchingProgram.SetUniform("detailNoiseWeight", erosionValue);
        rayMarchingProgram.SetUniform("shapeNoiseWeights", shapeNoiseWeights);
        rayMarchingProgram.SetUniform("shapeScale", shapeScale);
        rayMarchingProgram.SetUniform("detailScale", detailScale);
        rayMarchingProgram.SetUniform("cloudTopType", cloudTopType);
        rayMarchingProgram.SetUniform("cloudBottomType", cloudBottomType);

        // Uniforms — Ray Marching
        rayMarchingProgram.SetUniform("cloudMaxSteps", cloudMaxSteps);

        // Uniforms — Iluminação
        rayMarchingProgram.SetUniform("lightDirection", lightDirection);
        rayMarchingProgram.SetUniform("lightColor", lightColor);
        rayMarchingProgram.SetUniform("ambientColor", ambientColor);
        rayMarchingProgram.SetUniform("silver_intensity", silver_intensity);
        rayMarchingProgram.SetUniform("silver_spread", silver_spread);
        rayMarchingProgram.SetUniform("lightSteps", lightMaxSteps);
        rayMarchingProgram.SetUniform("phaseG", phaseG);
        rayMarchingProgram.SetUniform("extinctionCoef", extinctionCoef);
        rayMarchingProgram.SetUniform("scatteringCoef", scatteringCoef);
        rayMarchingProgram.SetUniform("ambientIntensity", ambientIntensity);
        rayMarchingProgram.SetUniform("precipitation", precipitation);

        // Uniforms — Performance Benchmark Controls
        rayMarchingProgram.SetUniform("enableDetailErosion", enableDetailErosion);
        rayMarchingProgram.SetUniform("enableLightMarching", enableLightMarching);
        rayMarchingProgram.SetUniform("enableBeersLaw", enableBeersLaw);
        rayMarchingProgram.SetUniform("enablePowderEffect", enablePowderEffect);
        rayMarchingProgram.SetUniform("enablePhaseFunction", enablePhaseFunction);
        rayMarchingProgram.SetUniform("enableSilverSheen", enableSilverSheen);

        // Uniforms — Vento / Tempo
        rayMarchingProgram.SetUniform("windDirection", windDirection);
        rayMarchingProgram.SetUniform("windSpeed", windSpeed * 0.1f);
        rayMarchingProgram.SetUniform("time", (float)glfwGetTime());


        camera.Matrix(rayMarchingProgram, "camMatrix");
        rayMarchingProgram.SetUniform("camPos", camera.Position);

        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, mainDepthTex);
        rayMarchingProgram.SetUniform("depthTex", 4);

        glm::mat4 invProjView = glm::inverse(camera.GetMatrix());
        rayMarchingProgram.SetUniform("invProjView", invProjView);

        drawScreenQuad();


        // ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);

        ImGui::Begin("Scene Viewport", nullptr, ImGuiWindowFlags_NoScrollbar);
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();
        if (viewportSize.x > 0 && viewportSize.y > 0) {
            // Update FBO if size changed
            if ((int)viewportSize.x != mainFBOWidth || (int)viewportSize.y != mainFBOHeight) {
                glDeleteFramebuffers(1, &mainFBO);
                glDeleteTextures(1, &mainColorTex);
                glDeleteTextures(1, &mainDepthTex);
                initMainFBO((int)viewportSize.x, (int)viewportSize.y);
                camera.SetAspectRatio((int)viewportSize.x, (int)viewportSize.y);
            }
            ImGui::Image((ImTextureID)(intptr_t)mainColorTex, viewportSize, ImVec2(0, 1), ImVec2(1, 0));
        }
        bool viewportFocused = ImGui::IsWindowFocused();
        bool viewportHovered = ImGui::IsWindowHovered();
        ImGui::End();

        ImGui::Begin("Cloud Generation");

        if (previewTarget == 2)
            renderTexturePreview2D(*slice2DShader, weatherTexture, previewChannel);
        else
            renderTexturePreview(*sliceShader,
                previewTarget == 0 ? shapeTexture : detailTexture,
                previewSlice, previewChannel);

        ImGui::SeparatorText("Visualization");
        ImGui::RadioButton("Shape", &previewTarget, 0); ImGui::SameLine();
        ImGui::RadioButton("Detail", &previewTarget, 1); ImGui::SameLine();
        ImGui::RadioButton("Weather", &previewTarget, 2);
        ImGui::RadioButton("RGBA", &previewChannel, 0); ImGui::SameLine();
        ImGui::RadioButton("R", &previewChannel, 1); ImGui::SameLine();
        ImGui::RadioButton("G", &previewChannel, 2); ImGui::SameLine();
        ImGui::RadioButton("B", &previewChannel, 3);
        if (previewTarget == 0) { ImGui::SameLine(); ImGui::RadioButton("A", &previewChannel, 4); }
        ImGui::Image((ImTextureID)(intptr_t)previewTex, ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));

        ImGui::SeparatorText("Configuration");
        switch (previewTarget) {
        case 0: //Shape
            if (previewTarget != 2) ImGui::SliderFloat("Slice Z", &previewSlice, 0.0f, 1.0f);
            if (ImGui::DragInt3("Shape Octaves", glm::value_ptr(shapeOctaves), 1, 2, 64)) needsUpdate = true;
            if (ImGui::DragFloat("Perlin Scale", &perlinScale, 0.1f, 1.0f, 20.0f))        needsUpdate = true;
            ImGui::DragFloat4("Shape Weights", glm::value_ptr(shapeNoiseWeights), 0.01f, 0.0f, 2.0f);
            if (ImGui::Checkbox("Invert Worley", &invertWorleyShape))
                needsUpdate = true;
            break;
        case 1: //Detail
            ImGui::SliderFloat("Slice Z", &previewSlice, 0.0f, 1.0f);
            if (ImGui::DragInt3("Detail Octaves", glm::value_ptr(detailOctaves), 1, 2, 64)) needsUpdate = true;
            if (ImGui::Checkbox("Invert Worley", &invertWorleyDetail))
                needsUpdate = true;
            break;
        case 2: //Weather
            if (ImGui::DragFloat("Coverage Scale", &coverageScale, 0.1f, 1.0f)) needsUpdate = true;
            if (ImGui::DragFloat("Height Scale", &heightScale, 0.1f, 1.0f, 16.0f)) needsUpdate = true;
            if (ImGui::DragFloat("Altitude Scale", &altitudeScale, 0.1f, 0.0f, 16.0f)) needsUpdate = true;
            if (ImGui::Button("Randomize Weather Map")) {
                weatherNoiseOffset = glm::vec2(dist(rng), dist(rng));
                needsUpdate = true;
            }
            break;
        }
        ImGui::End();

        ImGui::Begin("Volumetric Clouds");

        ImGui::SeparatorText("Wind");
        ImGui::DragFloat3("Direction", glm::value_ptr(windDirection), 0.01f, -1.0f, 1.0f);
        ImGui::DragFloat("Speed", &windSpeed, 0.01f);

        ImGui::SeparatorText("Atmosphere");
        ImGui::DragFloat("Planet Radius", &planetRadius, 100.0f, 1000.0f, 50000.0f);
        ImGui::DragFloat("Atmosphere Start", &atmosphereStart, 0.5f);
        ImGui::DragFloat("Atmosphere Height", &atmosphereHeight, 0.5f, 0.0f);
        ImGui::DragFloat("Atmosphere Max Depth", &atmosphereMaxDepth, 0.5f, 0.0f);
        ImGui::DragFloat("Weather Scale", &weatherScale, 1.0f, 0.1f);

        ImGui::SeparatorText("Density");
        ImGui::DragFloat("Cloud Top Type", &cloudTopType, 0.001f, 0.0f, 1.0f);
        ImGui::DragFloat("Cloud Bottom Type", &cloudBottomType, 0.001f, 0.0f, 1.0f);
        ImGui::DragFloat("Shape Scale", &shapeScale, 0.1f, 0.1f);
        ImGui::DragFloat("Detail Scale", &detailScale, 0.1f, 0.1f);
        ImGui::DragFloat("Erosion Weight", &erosionValue, 0.01f, 0.0f, 1.0f);

        ImGui::SeparatorText("Ray Marching");
        ImGui::DragInt("Max Steps", &cloudMaxSteps, 1, 1, 512);

        ImGui::SeparatorText("Lighting");
        ImGui::DragFloat3("Light Direction", glm::value_ptr(lightDirection), 0.01f, -1.0f, 1.0f);
        ImGui::ColorEdit3("Light Color", glm::value_ptr(lightColor));
        ImGui::ColorEdit3("Ambient Color", glm::value_ptr(ambientColor));
        ImGui::DragFloat("Phase Value", &phaseG, 0.01f, 0.0f, 0.999f);
        ImGui::DragFloat("Extinction", &extinctionCoef, 0.01f, 0.0f, 1.0f, "%.4f");
        ImGui::DragFloat("Ambient Intensity", &ambientIntensity, 0.01f, 0.1f, 5.0f);
        ImGui::DragFloat("Silver Intensity", &silver_intensity, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat("Silver Spread", &silver_spread, 0.01f, 0.1f, 1.0f);
        ImGui::DragInt("Light Steps", &lightMaxSteps, 1, 0, 16);

        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();

        ImGui::Begin("Skybox");

        auto skyboxNames = skyboxManager.getSkyboxNames();
        int skyboxIndex = skyboxManager.getCurrentIndex();

        ImGui::SeparatorText("Background Mode");
        ImGui::RadioButton("Skybox", (int*)&useSkybox, 1); ImGui::SameLine();
        ImGui::RadioButton("Gradient", (int*)&useSkybox, 0);

        if (useSkybox)
        {
            ImGui::Spacing();
            if (ImGui::Combo("##SkyboxSelector", &skyboxIndex, skyboxNames.data(), skyboxNames.size())) {
                skyboxManager.setCurrentSkybox(skyboxIndex);
                cubemapTexture = skyboxManager.getCurrentSkyboxTexture();
                currentSkyboxName = skyboxManager.getCurrentSkyboxName();
            }

            if (ImGui::Button("Rescan Skyboxes", ImVec2(-1, 0))) {
                skyboxManager.reloadAll();
                if (skyboxManager.getSkyboxCount() > 0) {
                    cubemapTexture = skyboxManager.getCurrentSkyboxTexture();
                    currentSkyboxName = skyboxManager.getCurrentSkyboxName();
                }
            }
        }
        else
        {
            ImGui::Spacing();
            ImGui::ColorEdit3("Top Color", glm::value_ptr(gradientColorTop));
            ImGui::ColorEdit3("Bottom Color", glm::value_ptr(gradientColorBottom));
        }

        ImGui::End();

        ImGui::Begin("Terrain");

        if (ImGui::Button("Load OBJ..."))
        {
            IGFD::FileDialogConfig config;
            config.path = ".";
            ImGuiFileDialog::Instance()->OpenDialog("ChooseOBJ", "Choose OBJ File", ".obj", config);
        }

        if (ImGuiFileDialog::Instance()->Display("ChooseOBJ"))
        {
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
                currentTerrainPath = filePath;
                terrainMesh = Mesh::Make(filePath);

                // Fallback: tenta carregar textura com mesmo nome do OBJ (.jpg ou .png)
                // Só é aplicada em submeshes que o MTL não forneceu textura
                if (terrainTexture != 0) { glDeleteTextures(1, &terrainTexture); terrainTexture = 0; }

                std::string base = filePath.substr(0, filePath.find_last_of('.'));
                for (const std::string& ext : { std::string(".jpg"), std::string(".png") }) {
                    std::string texPath = base + ext;
                    int w, h, ch;
                    unsigned char* data = stbi_load(texPath.c_str(), &w, &h, &ch, 0);
                    if (data) {
                        GLenum fmt = (ch == 4) ? GL_RGBA : GL_RGB;
                        glGenTextures(1, &terrainTexture);
                        glBindTexture(GL_TEXTURE_2D, terrainTexture);
                        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
                        glGenerateMipmap(GL_TEXTURE_2D);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                        stbi_image_free(data);
                        std::cout << "[Terrain] Fallback texture loaded: " << texPath << "\n";
                        break;
                    }
                }
                if (terrainTexture != 0)
                    terrainMesh->SetFallbackTexture(terrainTexture);
            }
            ImGuiFileDialog::Instance()->Close();

        }

        if (terrainMesh)
        {
            ImGui::ColorEdit3("Terrain Color", glm::value_ptr(terrainColor));
            ImGui::SeparatorText("Transform");
            ImGui::DragFloat3("Position", glm::value_ptr(terrainPosition), 0.1f);
            ImGui::DragFloat3("Scale", glm::value_ptr(terrainScale), 0.01f, 0.001f, 1000.0f);
            if (ImGui::Button("Reset Transform"))
            {
                terrainPosition = glm::vec3(0.0f);
                terrainScale = glm::vec3(1.0f);
            }
            ImGui::SeparatorText("Camera");
            ImGui::DragFloat("Far Plane", &cameraFar, 10.0f, 10.0f, 1000000.0f, "%.0f");
            if (ImGui::Button("Unload"))
            {
                terrainMesh = nullptr;
                currentTerrainPath.clear();
                if (terrainTexture != 0) { glDeleteTextures(1, &terrainTexture); terrainTexture = 0; }
            }
        }
        else
        {
            ImGui::TextDisabled("No mesh loaded.");
        }

        ImGui::End();


        ImGui::Begin("Presets");

        // Combo para selecionar preset
        auto presetNames = presetManager.getPresetNames();
        if (ImGui::Combo("##PresetSelector", &selectedPreset, presetNames.data(), presetNames.size())) {
            // Aplicar preset selecionado
            std::string presetTerrainPath;
            glm::vec3 presetCameraPos;
            glm::vec3 presetCameraOri;
            presetManager.applyPreset(selectedPreset,
                // Vento
                windDirection, windSpeed,
                // Atmosfera
                planetRadius, atmosphereStart, atmosphereHeight, atmosphereMaxDepth,
                // Densidade
                weatherScale, shapeScale, detailScale,
                erosionValue, shapeNoiseWeights,
                // Ray Marching
                cloudMaxSteps,
                // Iluminação
                lightDirection, lightColor, phaseG,
                scatteringCoef, extinctionCoef,
                ambientIntensity, precipitation, lightMaxSteps,
                // Noise
                shapeOctaves, detailOctaves, perlinScale,
                invertWorleyShape, invertWorleyDetail,
                // Weather
                coverageScale, heightScale, altitudeScale, presetCameraPos, presetCameraOri, currentSkyboxName,
                // terrain + visual
                presetTerrainPath, terrainPosition, terrainScale, useSkybox, gradientColorTop, gradientColorBottom, ambientIntensity,
                ambientColor, cloudTopType, cloudBottomType, silver_intensity, silver_spread,
                enableDetailErosion, enableLightMarching, enableBeersLaw, enablePowderEffect, enablePhaseFunction, enableSilverSheen
            );

            // Aplicar skybox do preset
            if (skyboxManager.setCurrentSkyboxByName(currentSkyboxName)) {
                /*if (cubemapTexture != 0) {
                    glDeleteTextures(1, &cubemapTexture);
                }*/
                cubemapTexture = skyboxManager.getCurrentSkyboxTexture();
            }

            // visual settings are written directly into `useSkybox`, `gradientColorTop`, `gradientColorBottom`, `ambientIntensity` by applyPreset

            // Apply camera from preset
            if (presetCameraPos != glm::vec3(0.0f) || presetCameraOri != glm::vec3(0.0f, 0.0f, -1.0f)) {
                camera.Position = presetCameraPos;
                camera.Orientation = presetCameraOri;
            }

            // If preset provided a terrainPath, load it (só se for diferente do já carregado)
            if (!presetTerrainPath.empty() && presetTerrainPath != currentTerrainPath) {
                if (terrainMesh) { terrainMesh = nullptr; }
                currentTerrainPath = presetTerrainPath;
                terrainMesh = Mesh::Make(presetTerrainPath);

                if (terrainTexture != 0) { glDeleteTextures(1, &terrainTexture); terrainTexture = 0; }
                std::string base = presetTerrainPath.substr(0, presetTerrainPath.find_last_of('.'));
                for (const std::string& ext : { std::string(".jpg"), std::string(".png") }) {
                    int w, h, ch;
                    unsigned char* data = stbi_load((base + ext).c_str(), &w, &h, &ch, 0);
                    if (data) {
                        GLenum fmt = (ch == 4) ? GL_RGBA : GL_RGB;
                        glGenTextures(1, &terrainTexture);
                        glBindTexture(GL_TEXTURE_2D, terrainTexture);
                        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
                        glGenerateMipmap(GL_TEXTURE_2D);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                        stbi_image_free(data);
                        std::cout << "[Terrain] Fallback texture loaded from preset: " << base + ext << "\n";
                        break;
                    }
                }
                if (terrainTexture != 0)
                    terrainMesh->SetFallbackTexture(terrainTexture);
            }

            needsUpdate = true;
        }

        ImGui::Separator();

        // Salvar novo preset
        ImGui::Text("Salvar Preset Atual:");
        ImGui::InputText("Nome", newPresetName, 64);

        if (ImGui::Button("Salvar", ImVec2(-1, 0))) {
            if (strlen(newPresetName) > 0) {
                presetManager.saveCurrentState(
                    std::string(newPresetName),
                    // Vento
                    windDirection, windSpeed,
                    // Atmosfera
                    planetRadius, atmosphereStart, atmosphereHeight, atmosphereMaxDepth,
                    // Densidade
                    weatherScale, shapeScale, detailScale,
                    erosionValue, shapeNoiseWeights,
                    // Ray Marching
                    cloudMaxSteps,
                    // Iluminação
                    lightDirection, lightColor, phaseG,
                    scatteringCoef, extinctionCoef,
                    ambientIntensity, precipitation, lightMaxSteps,
                    // Noise
                    shapeOctaves, detailOctaves, perlinScale,
                    invertWorleyShape, invertWorleyDetail,
                    // Weather
                    coverageScale, heightScale, altitudeScale, camera.Position, camera.Orientation, currentSkyboxName,
                    // terrain + visual
                    currentTerrainPath, terrainPosition, terrainScale, useSkybox, gradientColorTop, gradientColorBottom, ambientIntensity,
                    ambientColor, cloudTopType, cloudBottomType, silver_intensity, silver_spread,
                    enableDetailErosion, enableLightMarching, enableBeersLaw, enablePowderEffect, enablePhaseFunction, enableSilverSheen
                );

                // Limpa o input
                newPresetName[0] = '\0';

                // Atualiza o combo para mostrar o novo preset
                selectedPreset = presetManager.getPresetCount() - 1;
            }
        }

        ImGui::Separator();

        // Deletar preset (não pode deletar Default)
        if (selectedPreset > 0) {
            if (ImGui::Button("Deletar Preset Selecionado", ImVec2(-1, 0))) {
                presetManager.deletePreset(selectedPreset);
                selectedPreset = 0; // volta para Default
            }
        }

        ImGui::End();

        // ─── Performance Benchmark Window ─────────────────────────────────────
        ImGui::Begin("Performance Benchmark");

        ImGui::SeparatorText("Frame Statistics");
        ImGui::Text("FPS: %.1f", io.Framerate);
        ImGui::Text("Frame Time: %.3f ms", 1000.0f / io.Framerate);
        static int frameCount = 0;
        frameCount++;
        ImGui::Text("Frame Count: %d", frameCount);

        ImGui::Spacing();
        ImGui::SeparatorText("Technique Toggle");
        ImGui::Text("Enable/Disable techniques to measure impact:");
        ImGui::Separator();

        ImGui::Checkbox("##DetailErosion", &enableDetailErosion);
        ImGui::SameLine();
        ImGui::Text("Detail Erosion (high impact)");

        ImGui::Checkbox("##LightMarching", &enableLightMarching);
        ImGui::SameLine();
        ImGui::Text("Light Marching (very high impact)");

        ImGui::Checkbox("##BeersLaw", &enableBeersLaw);
        ImGui::SameLine();
        ImGui::Text("Beer's Law (high impact)");

        ImGui::Checkbox("##PowderEffect", &enablePowderEffect);
        ImGui::SameLine();
        ImGui::Text("Powder Effect (medium impact)");

        ImGui::Checkbox("##PhaseFunction", &enablePhaseFunction);
        ImGui::SameLine();
        ImGui::Text("Phase Function (medium impact)");

        ImGui::Checkbox("##SilverSheen", &enableSilverSheen);
        ImGui::SameLine();
        ImGui::Text("Silver Sheen (low impact)");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Current Configuration:");
        int activeCount = 0;
        if (enableDetailErosion) activeCount++;
        if (enableLightMarching) activeCount++;
        if (enableBeersLaw) activeCount++;
        if (enablePowderEffect) activeCount++;
        if (enablePhaseFunction) activeCount++;
        if (enableSilverSheen) activeCount++;
        ImGui::Text("Active Techniques: %d/6", activeCount);

        ImGui::End();



        // ImGui Render
        ImGui::Render();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT); // or use the window's actual size
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup);
        }

        // ─── Camera Input (only when viewport is focused/hovered) ─────────────
        camera.Inputs(window, &io, viewportFocused || viewportHovered);

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteFramebuffers(1, &mainFBO);
    glDeleteTextures(1, &mainColorTex);
    glDeleteTextures(1, &mainDepthTex);

    rayMarchingProgram.Delete();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Inicialização
// ═════════════════════════════════════════════════════════════════════════════

void initMainFBO(int width, int height) {
    glGenFramebuffers(1, &mainFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, mainFBO);

    glGenTextures(1, &mainColorTex);
    glBindTexture(GL_TEXTURE_2D, mainColorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mainColorTex, 0);

    glGenTextures(1, &mainDepthTex);
    glBindTexture(GL_TEXTURE_2D, mainDepthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, mainDepthTex, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Main FBO incomplete!\n";

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    mainFBOWidth = width;
    mainFBOHeight = height;
}

void initScreenQuad()
{
    VAO quadVAO;
    quadVAO.Bind();

    GLfloat verts[] = {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    quadID = quadVAO.ID;
    VBO quadVBO(verts, sizeof(verts));
    quadVAO.LinkAttrib(quadVBO, 0, 2, GL_FLOAT, 4 * sizeof(float), (void*)0);
    quadVAO.LinkAttrib(quadVBO, 1, 2, GL_FLOAT, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    quadVAO.Unbind();
}

void initTextures()
{
    auto create3DTex = [](GLuint& id, glm::ivec3 res) {
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_3D, id);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, res.x, res.y, res.z, 0, GL_RGBA, GL_FLOAT, NULL);
        };

    create3DTex(shapeTexture, glm::ivec3(128, 32, 128));
    create3DTex(detailTexture, glm::ivec3(32, 32, 32));

    // Weather Map — 2D
    glGenTextures(1, &weatherTexture);
    glBindTexture(GL_TEXTURE_2D, weatherTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 1024, 1024, 0, GL_RGBA, GL_FLOAT, NULL);
}

void initTexturePreview()
{
    glGenTextures(1, &previewTex);
    glBindTexture(GL_TEXTURE_2D, previewTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 256, 256, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenFramebuffers(1, &previewFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, previewFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, previewTex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ═════════════════════════════════════════════════════════════════════════════
//  Compute Shaders
// ═════════════════════════════════════════════════════════════════════════════

void updateNoiseSSBO(GLuint ssbo, glm::ivec3 octaves, glm::ivec3& offsetsOut)
{
    std::vector<glm::vec4> points;
    int offset = 0;

    for (int i = 0; i < 3; i++) {
        offsetsOut[i] = offset;
        WorleyNoise3D gen(octaves[i]);
        gen.GeneratePoints();
        auto& pts = gen.getPoints();
        points.insert(points.end(), pts.begin(), pts.end());
        offset += pts.size();
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, points.size() * sizeof(glm::vec4), points.data(), GL_STATIC_DRAW);
}

void dispatchNoiseCompute(Shader& shader, GLuint tex, glm::ivec3 res, glm::ivec3 octaves, glm::ivec3 offsets, bool isShape, bool invertWorleyNoise)
{
    shader.Activate();
    shader.SetUniform("numCells", octaves);
    shader.SetUniform("offsets", offsets);
    shader.SetUniform("isShape", isShape);
    shader.SetUniform("perlinScale", perlinScale);
    shader.SetUniform("invertWorley", invertWorleyNoise);

    glBindImageTexture(0, tex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, isShape ? ssboShape : ssboDetail);
    glDispatchCompute(ceilDiv(res.x, 8), ceilDiv(res.y, 8), ceilDiv(res.z, 8));
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
}

void dispatchWeatherCompute(Shader& shader, GLuint tex)
{
    shader.Activate();
    glBindImageTexture(0, tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glDispatchCompute(ceilDiv(1024, 8), ceilDiv(1024, 8), 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
}


// ═════════════════════════════════════════════════════════════════════════════
//  Preview de Textura
// ═════════════════════════════════════════════════════════════════════════════

void renderTexturePreview(Shader& shader, GLuint tex3D, float slice, int channel)
{
    glBindFramebuffer(GL_FRAMEBUFFER, previewFBO);
    glViewport(0, 0, 256, 256);

    shader.Activate();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, tex3D);
    shader.SetUniform("noiseTex", 0);
    shader.SetUniform("slice", slice);
    shader.SetUniform("channel", channel);
    drawScreenQuad();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    int w, h;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &w, &h);
    glViewport(0, 0, w, h);
}

void renderTexturePreview2D(Shader& shader, GLuint tex2D, int channel)
{
    glBindFramebuffer(GL_FRAMEBUFFER, previewFBO);
    glViewport(0, 0, 256, 256);

    shader.Activate();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex2D);
    shader.SetUniform("noiseTex", 0);
    shader.SetUniform("channel", channel);
    drawScreenQuad();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    int w, h;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &w, &h);
    glViewport(0, 0, w, h);
}

void drawScreenQuad()
{
    glBindVertexArray(quadID);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

// ═════════════════════════════════════════════════════════════════════════════
//  Utilitários
// ═════════════════════════════════════════════════════════════════════════════

unsigned int loadCubemap(std::vector<std::string> faces)
{
    unsigned int id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, id);

    int w, h, ch;
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &w, &h, &ch, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else {
            std::cout << "Cubemap failed: " << faces[i] << "\n";
            stbi_image_free(data);
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    return id;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}