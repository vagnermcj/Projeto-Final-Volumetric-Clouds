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

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "shaderClass.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "Camera.h"
#include "WorleyNoise3D.h"

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 800;

static GLuint quadID;

GLuint shapeTexture, detailTexture, weatherTexture;
GLuint ssboShape, ssboDetail, ssboAltitude;
GLuint previewFBO, previewTex;

Shader* sliceShader;
Shader* slice2DShader;

glm::ivec3 shapeOctaves(2, 4, 8);
glm::ivec3 detailOctaves(8, 16, 32);
float      perlinScale = 1.0f;
bool       needsUpdate = true;

struct NoiseBufferData {
    std::vector<glm::vec3> allPoints;
    glm::ivec3 offsets;
};

struct AltitudePoint {
    glm::vec2 position;
    float     value;
    float     radius;
};

// ─── Protótipos ───────────────────────────────────────────────────────────────
void         initTextures();
void         initTexturePreview();
void         initScreenQuad();
void         drawScreenQuad();
void         framebuffer_size_callback(GLFWwindow* window, int width, int height);
void         updateNoiseSSBO(GLuint ssbo, glm::ivec3 octaves, glm::ivec3& offsetsOut);
void         dispatchNoiseCompute(Shader& shader, GLuint tex, glm::ivec3 res, glm::ivec3 octaves, glm::ivec3 offsets, bool isShape);
void         dispatchWeatherCompute(Shader& shader, GLuint tex);
void         renderTexturePreview(Shader& shader, GLuint tex3D, float slice, int channel);
void         renderTexturePreview2D(Shader& shader, GLuint tex2D, int channel);
void         generateAltitudePoints(int count, float minR, float maxR);
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
    gladLoadGL();
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // ─── Shaders ──────────────────────────────────────────────────────────────
    Shader rayMarchingProgram("RayMarch.vert", "RayMarch.frag");
    Shader noiseCompute("NoiseCompute.glsl");
    Shader weatherCompute("WeatherCompute.glsl");
    sliceShader = new Shader("RayMarch.vert", "SlicePreview.frag");
    slice2DShader = new Shader("RayMarch.vert", "SlicePreview2D.frag");

    // ─── Recursos ─────────────────────────────────────────────────────────────
    initTextures();
    initTexturePreview();
    initScreenQuad();
    glGenBuffers(1, &ssboShape);
    glGenBuffers(1, &ssboDetail);

    std::vector<std::string> faces = {
        "images/px.png", "images/nx.png",
        "images/py.png", "images/ny.png",
        "images/pz.png", "images/nz.png"
    };
    unsigned int cubemapTexture = loadCubemap(faces);

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

    // ─── Parâmetros de Iluminação ─────────────────────────────────────────────
    glm::vec3 lightDirection(-0.08f, 0.35f, 1.0f);
    glm::vec3 lightColor(1.0f);
    float phaseG = 0.95f;
    glm::vec3 scatteringColor = glm::vec3(0.8f, 0.9f, 1.0f);
    glm::vec3 absorptionColor = glm::vec3(0.05f, 0.05f, 0.05f);
    glm::vec3 ambientColor = glm::vec3(0.8f, 0.9f, 1.0f); 
    float precipitation = 1.0f;
    int lightMaxSteps = 3;

    // ─── Parâmetros de Vento ──────────────────────────────────────────────────
    glm::vec3 windDirection(1.0f, 0.0f, 1.0f);
    float     windSpeed = 0.01f;

    // ─── Parâmetros da Atmosfera ──────────────────────────────────────────────
    float planetRadius = 6000.0f;
    float atmosphereStart = 100.0f;
    float atmosphereHeight = 100.0f;
	float atmosphereMaxDepth = 200.0f;
    float innerCloudRadius, outerCloudRadius;

    // ─── Parâmetros de Densidade ──────────────────────────────────────────────
    float weatherScale = 500.0f;
    float maxCloudHeight = 25.0f;
    float maxCloudAltitude = 80.0f;
    float detailNoiseWeight = 0.15f;
    glm::vec4 shapeNoiseWeights(1.0f, 0.625f, 0.25f, 0.125f);
    float shapeScale = 200.0f; //Ainda na duvida entre 500 ou 0.001f
	float detailScale = 50.0f; //Ainda na duvida entre 500 ou 0.001f
    int cloudMaxSteps = 128;

    // ─── Parâmetros do Weather Map ────────────────────────────────────────────
    float coverageScale = 3.0f;
    float heightScale = 1.5f;
    float altitudeScale = 0.8f;
    float coverageMin = 0.4f;
    float coverageMax = 0.7f;
    int   altitudePointCount = 3;
    float altitudeBlobMinRadius = 0.05f;
    float altitudeBlobMaxRadius = 0.15f;

    generateAltitudePoints(altitudePointCount, altitudeBlobMinRadius, altitudeBlobMaxRadius);

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
            dispatchNoiseCompute(noiseCompute, shapeTexture, glm::ivec3(128, 32, 128), shapeOctaves, sOff, true);
            dispatchNoiseCompute(noiseCompute, detailTexture, glm::ivec3(32, 32, 32), detailOctaves, dOff, false);

            weatherCompute.Activate();
            weatherCompute.SetUniform("coverageScale", coverageScale);
            weatherCompute.SetUniform("heightScale", heightScale);
            weatherCompute.SetUniform("altitudeScale", altitudeScale);
            weatherCompute.SetUniform("coverageMin", coverageMin);
            weatherCompute.SetUniform("coverageMax", coverageMax);
            weatherCompute.SetUniform("altitudePointCount", altitudePointCount);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboAltitude);
            dispatchWeatherCompute(weatherCompute, weatherTexture);

            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
            needsUpdate = false;
        }

        // ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

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
        case 0:
            if (previewTarget != 2) ImGui::SliderFloat("Slice Z", &previewSlice, 0.0f, 1.0f);
            if (ImGui::DragInt3("Shape Octaves", glm::value_ptr(shapeOctaves), 1, 2, 64)) needsUpdate = true;
            if (ImGui::DragFloat("Perlin Scale", &perlinScale, 0.1f, 1.0f, 20.0f))        needsUpdate = true;
            ImGui::DragFloat4("Shape Weights", glm::value_ptr(shapeNoiseWeights), 0.01f, 0.0f, 2.0f);
            break;
        case 1:
            ImGui::SliderFloat("Slice Z", &previewSlice, 0.0f, 1.0f);
            if (ImGui::DragInt3("Detail Octaves", glm::value_ptr(detailOctaves), 1, 2, 64)) needsUpdate = true;
            break;
        case 2:
            if (ImGui::DragFloat("Coverage Scale", &coverageScale, 0.1f, 1.0f, 16.0f)) needsUpdate = true;
            if (ImGui::DragFloat("Height Scale", &heightScale, 0.1f, 1.0f, 16.0f)) needsUpdate = true;
            if (ImGui::DragFloat("Altitude Scale", &altitudeScale, 0.1f, 1.0f, 16.0f)) needsUpdate = true;
            if (ImGui::DragFloat("Coverage Min", &coverageMin, 0.01f, 0.0f, 1.0f)) needsUpdate = true;
            if (ImGui::DragFloat("Coverage Max", &coverageMax, 0.01f, 0.0f, 1.0f)) needsUpdate = true;
            ImGui::SeparatorText("Altitude Blobs");
            ImGui::DragFloat("Blob Min Radius", &altitudeBlobMinRadius, 0.01f, 0.01f, 0.5f);
            ImGui::DragFloat("Blob Max Radius", &altitudeBlobMaxRadius, 0.01f, 0.01f, 0.5f);
            if (ImGui::DragInt("Point Count", &altitudePointCount, 1, 1, 64)) {
                generateAltitudePoints(altitudePointCount, altitudeBlobMinRadius, altitudeBlobMaxRadius);
                needsUpdate = true;
            }
            if (ImGui::Button("Randomize")) {
                generateAltitudePoints(altitudePointCount, altitudeBlobMinRadius, altitudeBlobMaxRadius);
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
        ImGui::DragFloat("Max Cloud Height", &maxCloudHeight, 0.5f, 0.1f, atmosphereHeight);
        ImGui::DragFloat("Max Cloud Altitude", &maxCloudAltitude, 0.5f, 0.0f, atmosphereHeight);
        ImGui::DragFloat("Shape Scale", &shapeScale, 0.1f, 0.1f);
        ImGui::DragFloat("Detail Scale", &detailScale, 0.1f, 0.1f);
        ImGui::DragFloat("Erosion Weight", &detailNoiseWeight, 0.01f, 0.0f, 1.0f);

        ImGui::SeparatorText("Ray Marching");
        ImGui::DragInt("Max Steps", &cloudMaxSteps, 1, 1, 512);

        ImGui::SeparatorText("Lighting");
        ImGui::DragFloat3("Light Direction", glm::value_ptr(lightDirection), 0.01f, -1.0f, 1.0f);
        ImGui::ColorEdit3("Light Color", glm::value_ptr(lightColor));
        ImGui::DragFloat("Phase Value", &phaseG, 0.01f, 0.0f, 0.999f);
        ImGui::ColorEdit3("Scattering Color", glm::value_ptr(scatteringColor));
        ImGui::ColorEdit3("Absorption Color", glm::value_ptr(absorptionColor));
		ImGui::ColorEdit3("Ambient Color", glm::value_ptr(ambientColor));
		ImGui::DragFloat("Precipitation", &precipitation, 0.01f, 0.01f, 1.0f);
        ImGui::DragInt("Light Steps", &lightMaxSteps, 1, 0, 16);

        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();

        // Render
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);

        rayMarchingProgram.Activate();

        // Texturas
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_3D, shapeTexture);   rayMarchingProgram.SetUniform("shapeNoise", 0);
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_3D, detailTexture);  rayMarchingProgram.SetUniform("detailNoise", 1);
        glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, weatherTexture); rayMarchingProgram.SetUniform("weatherMap", 2);
        glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture); rayMarchingProgram.SetUniform("skybox", 3);

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
        rayMarchingProgram.SetUniform("maxCloudHeight", maxCloudHeight);
        rayMarchingProgram.SetUniform("maxCloudAltitude", maxCloudAltitude);
        rayMarchingProgram.SetUniform("detailNoiseWeight", detailNoiseWeight);
        rayMarchingProgram.SetUniform("shapeNoiseWeights", shapeNoiseWeights);
        rayMarchingProgram.SetUniform("shapeScale", shapeScale);
        rayMarchingProgram.SetUniform("detailScale", detailScale);

        // Uniforms — Ray Marching
        rayMarchingProgram.SetUniform("cloudMaxSteps", cloudMaxSteps);

        // Uniforms — Iluminação
        rayMarchingProgram.SetUniform("lightDirection", lightDirection);
        rayMarchingProgram.SetUniform("lightColor", lightColor);
        rayMarchingProgram.SetUniform("lightSteps", lightMaxSteps);
        rayMarchingProgram.SetUniform("phaseG", phaseG);
		rayMarchingProgram.SetUniform("scatteringColor", scatteringColor);
		rayMarchingProgram.SetUniform("absorptionColor", absorptionColor);
		rayMarchingProgram.SetUniform("ambientColor", ambientColor);
		rayMarchingProgram.SetUniform("precipitation", precipitation);

        // Uniforms — Vento / Tempo
        rayMarchingProgram.SetUniform("windDirection", windDirection);
        rayMarchingProgram.SetUniform("windSpeed", windSpeed * 0.1f);
        rayMarchingProgram.SetUniform("time", (float)glfwGetTime());

        drawScreenQuad();

        // Câmera
        camera.Inputs(window, &io);
        camera.updateMatrix(45.0f, 0.1f, 100.0f);
        camera.Matrix(rayMarchingProgram, "camMatrix");
        rayMarchingProgram.SetUniform("camPos", camera.Position);

        // ImGui Render
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup);
        }

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    rayMarchingProgram.Delete();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Inicialização
// ═════════════════════════════════════════════════════════════════════════════

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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 512, 512, 0, GL_RGBA, GL_FLOAT, NULL);
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

void dispatchNoiseCompute(Shader& shader, GLuint tex, glm::ivec3 res, glm::ivec3 octaves, glm::ivec3 offsets, bool isShape)
{
    shader.Activate();
    shader.SetUniform("numCells", octaves);
    shader.SetUniform("offsets", offsets);
    shader.SetUniform("isShape", isShape);
    shader.SetUniform("perlinScale", perlinScale);

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

void generateAltitudePoints(int count, float minR, float maxR)
{
    std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_real_distribution<float> pos(0.0f, 1.0f);
    std::uniform_real_distribution<float> rad(minR, maxR);

    std::vector<AltitudePoint> points(count);
    for (auto& p : points) {
        p.position = glm::vec2(pos(rng), pos(rng));
        p.value = pos(rng);
        p.radius = rad(rng);
    }

    glGenBuffers(1, &ssboAltitude);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboAltitude);
    glBufferData(GL_SHADER_STORAGE_BUFFER, points.size() * sizeof(AltitudePoint), points.data(), GL_STATIC_DRAW);
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