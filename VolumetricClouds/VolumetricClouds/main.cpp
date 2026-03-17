#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <random>
#include <chrono>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>


#include"shaderClass.h"
#include"VAO.h"
#include"VBO.h"
#include"EBO.h"
#include"Floor.h"
#include"Cube.h"
#include"Camera.h"
#include"WorleyNoise3D.h"


// Configuracoes Globais
static GLuint quadID;
const unsigned int SCR_WIDTH = 800; //
const unsigned int SCR_HEIGHT = 800; //

// IDs de Recursos
GLuint shapeTexture, detailTexture, weatherTexture;
GLuint ssboShape, ssboDetail, ssboAltitude;
GLuint quadVAO;

// Preview de Textura
GLuint previewFBO;
GLuint previewTex;
Shader* sliceShader;  
Shader* slice2DShader; //Weather Map 

// Parametros de Nuvem
glm::ivec3 shapeOctaves(4, 8, 16);
glm::ivec3 detailOctaves(4, 8, 16);
float perlinScale = 1.0f;
bool needsUpdate = true;

// Estrutura para gerenciar pontos no SSBO
struct NoiseBufferData {
	std::vector<glm::vec3> allPoints;
	glm::ivec3 offsets;
};

//Estrutura para ponto no altitude map
struct AltitudePoint {
	glm::vec2 position;
	float     value;
	float     radius;  // tamanho da mancha em UV [0,1]
};

// Prototipos
void initTextures();
void initTexturePreview();
void renderTexturePreview(Shader& sliceShader, GLuint tex3D, float slice, int channel);
void initScreenQuad();
void updateNoiseSSBO(GLuint ssbo, glm::ivec3 octaves, glm::ivec3& offsetsOut);
void dispatchNoiseCompute(Shader& computeShader, GLuint targetTex, glm::ivec3 res, glm::ivec3 octaves, glm::ivec3 offsets, bool isShape);
void dispatchWeatherCompute(Shader& computeShader, GLuint targetTex);
void renderTexturePreview2D(Shader& shader, GLuint tex2D, int channel);
void drawScreenQuad();
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void generateAltitudePoints(int count,float minR, float maxR);
unsigned int loadCubemap(std::vector<std::string> faces);

int main()
{
	// Initialize GLFW
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Clouds", NULL, NULL);
	// Error check if the window fails to create
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}


	glfwMakeContextCurrent(window);
	gladLoadGL();
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// Shaders
	Shader rayMarchingProgram("RayMarch.vert", "RayMarch.frag");
	Shader noiseCompute("NoiseCompute.glsl");
	sliceShader = new Shader("RayMarch.vert", "SlicePreview.frag");
	slice2DShader = new Shader("RayMarch.vert", "SlicePreview2D.frag");
	Shader weatherCompute("WeatherCompute.glsl");

	// Inicializar Recursos
	initTextures();
	initTexturePreview();
	initScreenQuad();
	glGenBuffers(1, &ssboShape);
	glGenBuffers(1, &ssboDetail);

	//Skybox Setup
	std::vector<std::string> faces = {
		"images/right.jpg", "images/left.jpg", "images/top.jpg", "images/bottom.jpg", "images/front.jpg", "images/back.jpg"
	};
	unsigned int cubemapTexture = loadCubemap(faces);

	

	//ImGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 430 core");

	//Camera
	Camera camera(SCR_WIDTH, SCR_HEIGHT, glm::vec3(0.0f, 0.0f, 5.0f));


	//Parametros
	glm::vec3 lightDirection = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec3 backgroundColor = glm::vec3(0.85, 0.9, 1.0);
	glm::vec3 cloudPosition = glm::vec3(1.0f, 0.0f, -2.0f);
	glm::vec3 cloudScale = glm::vec3(30.0f, 1.0f, 5.0f);
	glm::vec3 windDirection = glm::vec3(1.0f, 0.0f, 1.0f);
	glm::vec4 shapeNoiseWeights = glm::vec4(1.0f, 0.625f, 0.25f, 0.125f);

	float cloudMinCoverage = 0.5f;
	float weatherScale = 50.0f; 
	float scatteringCoefficient = 0.8f; 
	float absorptionCoefficient = 0.05; 
	float cloudStepSize = 0.5;
	float cloudCoverage = 0.5f; 
	int cloudMaxSteps = 128; 
	int lightMaxSteps = 3;
	float windSpeed = 0.01f;
	float atmosphereStart = 100.0f;
	float atmosphereHeight = 100.0f;

	// Parametros weather map
	float coverageScale = 3.0f;
	float heightScale = 1.5f;
	float altitudeScale = 0.8f;
	float coverageMin = 0.4f;
	float coverageMax = 0.7f;
	int altitudePointCount = 3;
	float maxCloudHeight = 10.0f;
	float maxCloudAltitude = 80.0f;
	float altitudeBlobMinRadius = 0.05f;
	float altitudeBlobMaxRadius = 0.15f;

	static float  previewSlice = 0.5f;
	static int    previewChannel = 0;
	static int    previewTarget = 0; // 0=Shape, 1=Detail, 2=Weather

	
	generateAltitudePoints(altitudePointCount, altitudeBlobMinRadius, altitudeBlobMaxRadius);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Texturas do compute shader
		if (needsUpdate) {
			glm::ivec3 sOff, dOff;
			updateNoiseSSBO(ssboShape, shapeOctaves, sOff);
			updateNoiseSSBO(ssboDetail, detailOctaves, dOff);

			dispatchNoiseCompute(noiseCompute, shapeTexture, glm::ivec3(128, 32, 128), shapeOctaves, sOff, true);

			dispatchNoiseCompute(noiseCompute, detailTexture, glm::ivec3(32, 32, 32), detailOctaves, dOff, false);

			//Gerar Weather Map
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

		ImGui::RadioButton("Shape", &previewTarget, 0);
		ImGui::SameLine();
		ImGui::RadioButton("Detail", &previewTarget, 1);
		ImGui::SameLine();
		ImGui::RadioButton("Weather", &previewTarget, 2);
		ImGui::RadioButton("RGBA", &previewChannel, 0); ImGui::SameLine();
		ImGui::RadioButton("R", &previewChannel, 1); ImGui::SameLine();
		ImGui::RadioButton("G", &previewChannel, 2); ImGui::SameLine();
		ImGui::RadioButton("B", &previewChannel, 3);
		if (previewTarget == 0)
		{
			ImGui::SameLine();
			ImGui::RadioButton("A", &previewChannel, 4);
		}
		// Agora sim passa a textura 2D renderizada
		ImGui::Image((ImTextureID)(intptr_t)previewTex, ImVec2(256, 256),
			ImVec2(0, 1),
			ImVec2(1, 0));


		ImGui::SeparatorText("Configuration");
		switch (previewTarget) {
			case 0: // Shape
				ImGui::SliderFloat("Slice Z", &previewSlice, 0.0f, 1.0f);
				if (ImGui::DragInt3("Shape Octaves", glm::value_ptr(shapeOctaves), 1, 2, 64)) needsUpdate = true;
				if (ImGui::DragFloat("Perlin Scale", &perlinScale, 0.1f, 1.0f, 20.0f)) needsUpdate = true;
				ImGui::DragFloat4("Shape Weights", glm::value_ptr(shapeNoiseWeights), 0.01f, 0.0f, 2.0f);
				break;
			case 1: //Detail
				ImGui::SliderFloat("Slice Z", &previewSlice, 0.0f, 1.0f);
				if (ImGui::DragInt3("Detail Octaves", glm::value_ptr(detailOctaves), 1, 2, 64)) needsUpdate = true;
				break;
			case 2: //Weather
				if (ImGui::DragFloat("Coverage Scale", &coverageScale, 1.0f, 1.0f, 16.0f)) needsUpdate = true;
				if (ImGui::DragFloat("Height Scale", &heightScale, 1.0f, 1.0f, 16.0f)) needsUpdate = true;
				if (ImGui::DragFloat("Altitude Scale", &altitudeScale, 1.0f, 1.0f, 16.0f)) needsUpdate = true;
				if (ImGui::DragFloat("Coverage Min", &coverageMin, 0.01f, 0.0f, 1.0f)) needsUpdate = true;
				if (ImGui::DragFloat("Coverage Max", &coverageMax, 0.01f, 0.0f, 1.0f)) needsUpdate = true;
				ImGui::DragFloat("Blob Min Radius", &altitudeBlobMinRadius, 0.01f, 0.01f, 0.5f);
				ImGui::DragFloat("Blob Max Radius", &altitudeBlobMaxRadius, 0.01f, 0.01f, 0.5f);
				ImGui::SeparatorText("Altitude Blobs");
				if (ImGui::DragInt("Altitude Point Count", &altitudePointCount, 1, 1, 64)) {
					generateAltitudePoints(altitudePointCount, altitudeBlobMinRadius, altitudeBlobMaxRadius);
					needsUpdate = true;
				}
				if (ImGui::Button("Randomize Altitude")) {
					generateAltitudePoints(altitudePointCount, altitudeBlobMinRadius, altitudeBlobMaxRadius);
					needsUpdate = true;
				}
				break;
		}
		
		ImGui::End();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		int w, h;
		glfwGetFramebufferSize(window, &w, &h);
		glViewport(0, 0, w, h);

		rayMarchingProgram.Activate();

		// Bind das texturas para o Fragment Shader (RayMarching)
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, shapeTexture);
		rayMarchingProgram.SetUniform("shapeNoise", 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_3D, detailTexture);
		rayMarchingProgram.SetUniform("detailNoise", 1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, weatherTexture);
		rayMarchingProgram.SetUniform("weatherMap", 2);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
		rayMarchingProgram.SetUniform("skybox", 3);


		ImGui::Begin("Volumetric Clouds");

		ImGui::SeparatorText("General Setup");
		ImGui::ColorEdit3("Background Color", glm::value_ptr(backgroundColor));
		ImGui::DragFloat3("Cloud Position", glm::value_ptr(cloudPosition), 0.1f);
		ImGui::DragFloat3("Cloud Scale", glm::value_ptr(cloudScale), 0.1f);
		ImGui::DragFloat3("Wind Direction (Normalized)", glm::value_ptr(windDirection), 0.01f, -1.0f, 1.0f);
		ImGui::DragFloat("Wind Speed", &windSpeed, 0.01f);

		ImGui::SeparatorText("Atmosphere");

		ImGui::DragFloat("Atmosphere Start", &atmosphereStart, 0.5f);
		ImGui::DragFloat("Atmosphere Height", &atmosphereHeight, 0.5f, 0.0f);
		ImGui::DragFloat("Cloud Coverage", &cloudCoverage, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("Cloud Min Value", &cloudMinCoverage, 0.005f, 0.0f, 1.0f);
		ImGui::DragFloat("Weather Scale", &weatherScale, 1.0f, 0.1f);
		ImGui::DragFloat("Max Cloud Height", &maxCloudHeight, 0.5f, 0.1f, atmosphereHeight);
		ImGui::DragFloat("Max Cloud Altitude", &maxCloudAltitude, 0.5f, 0.0f, atmosphereHeight);


		ImGui::SeparatorText("Ray Marching");
		ImGui::DragInt("Cloud Max Steps", &cloudMaxSteps, 1, 0, ImGuiSliderFlags_AlwaysClamp);
		ImGui::DragFloat("Cloud Step Size", &cloudStepSize, 0.001f, 0.001f, ImGuiSliderFlags_AlwaysClamp);

		ImGui::SeparatorText("Lightning");
		ImGui::DragFloat3("Light Direction (Normalized)", glm::value_ptr(lightDirection), 0.01f, 0.0f, 1.0f);
		ImGui::ColorEdit3("Light Color", glm::value_ptr(lightColor));
		ImGui::DragFloat("Absorption Coefficient", &absorptionCoefficient, 0.001f, 0.0f, 1.0f);
		ImGui::DragInt("Light Max Steps", &lightMaxSteps, 1, 0, 5, "%d", ImGuiSliderFlags_AlwaysClamp);
		ImGui::DragFloat("Scattering Coefficient", &scatteringCoefficient, 0.01f, 0.0f, 1.0f);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

		ImGui::End();



		//Passing Uniforms
		float t = glfwGetTime();
		rayMarchingProgram.SetUniform("lightDirection", lightDirection);
		rayMarchingProgram.SetUniform("lightColor", lightColor);
		rayMarchingProgram.SetUniform("lightSteps", lightMaxSteps);
		rayMarchingProgram.SetUniform("backgroundColor", backgroundColor);
		rayMarchingProgram.SetUniform("absorptionCoefficient", absorptionCoefficient);
		rayMarchingProgram.SetUniform("cloudStepSize", cloudStepSize);
		rayMarchingProgram.SetUniform("cloudMaxSteps", cloudMaxSteps);
		rayMarchingProgram.SetUniform("cloudPosition", cloudPosition);
		rayMarchingProgram.SetUniform("cloudScale", cloudScale);
		rayMarchingProgram.SetUniform("windDirection", windDirection);
		rayMarchingProgram.SetUniform("windSpeed", windSpeed);
		rayMarchingProgram.SetUniform("time", t);
		rayMarchingProgram.SetUniform("cloudCoverage", cloudCoverage);
		rayMarchingProgram.SetUniform("cloudMinCoverage", cloudMinCoverage);
		rayMarchingProgram.SetUniform("weatherScale", weatherScale);
		rayMarchingProgram.SetUniform("scatteringCoefficient", scatteringCoefficient);
		rayMarchingProgram.SetUniform("atmosphereStart", atmosphereStart);
		rayMarchingProgram.SetUniform("atmosphereHeight", atmosphereHeight);
		rayMarchingProgram.SetUniform("shapeNoiseWeights", shapeNoiseWeights);
		rayMarchingProgram.SetUniform("maxCloudHeight",   maxCloudHeight);
		rayMarchingProgram.SetUniform("maxCloudAltitude", maxCloudAltitude);

		drawScreenQuad();
		//Camera Update
		camera.Inputs(window, &io);
		camera.updateMatrix(45.0f, 0.1f, 100.0f); //Projection * view
		camera.Matrix(rayMarchingProgram, "camMatrix");
		glUniform3f(glGetUniformLocation(rayMarchingProgram.ID, "camPos"), camera.Position.x, camera.Position.y, camera.Position.z);


		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		//int display_w, display_h;
		//glfwGetFramebufferSize(window, &display_w, &display_h);
		//glViewport(0, 0, display_w, display_h);


		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
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

void initScreenQuad()
{
	VAO quadVAO;
	quadVAO.Bind();

	GLfloat quadVertices[] = {
		// pos      // texcoords (se quiser usar depois)
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,

		-1.0f,  1.0f,  0.0f, 1.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		 1.0f,  1.0f,  1.0f, 1.0f
	};

	quadID = quadVAO.ID;

	VBO quadVBO(quadVertices, sizeof(quadVertices));
	quadVAO.LinkAttrib(quadVBO, 0, 2, GL_FLOAT, 4 * sizeof(float), (void*)0); //Coord
	quadVAO.LinkAttrib(quadVBO, 1, 2, GL_FLOAT, 4 * sizeof(float), (void*)(2 * sizeof(float))); // texcoords


	quadVAO.Unbind();
}

void initTextures() {
	auto create3DTex = [](GLuint& id, glm::ivec3 res, GLenum format) {
		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_3D, id);
		// Filtros e Wrap
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
		// Aloca espa�o na GPU sem dados iniciais (NULL)
		glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, res.x, res.y, res.z, 0, GL_RGBA, GL_FLOAT, NULL);
		};

	create3DTex(shapeTexture, glm::ivec3(128, 32, 128), GL_RGBA32F);
	create3DTex(detailTexture, glm::ivec3(32, 32, 32), GL_RGBA32F);

	// Weather Map
	glGenTextures(1, &weatherTexture);
	glBindTexture(GL_TEXTURE_2D, weatherTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 512, 512, 0, GL_RGBA, GL_FLOAT, NULL);
}

void initTexturePreview() {
	// Cria textura 2D de destino
	glGenTextures(1, &previewTex);
	glBindTexture(GL_TEXTURE_2D, previewTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 256, 256, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Cria FBO
	glGenFramebuffers(1, &previewFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, previewFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, previewTex, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void renderTexturePreview(Shader& sliceShader, GLuint tex3D, float slice, int channel) {
	glBindFramebuffer(GL_FRAMEBUFFER, previewFBO);
	glViewport(0, 0, 256, 256);

	sliceShader.Activate();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, tex3D);
	sliceShader.SetUniform("noiseTex", 0);
	sliceShader.SetUniform("slice", slice);
	sliceShader.SetUniform("channel", channel);

	drawScreenQuad();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	int w, h;
	glfwGetFramebufferSize(glfwGetCurrentContext(), &w, &h);
	glViewport(0, 0, w, h);
}

void updateNoiseSSBO(GLuint ssbo, glm::ivec3 octaves, glm::ivec3& offsetsOut) {
	std::vector<glm::vec4> combinedPoints;
	int currentOffset = 0;

	for (int i = 0; i < 3; i++) {
		offsetsOut[i] = currentOffset;
		WorleyNoise3D gen(octaves[i]);
		gen.GeneratePoints();
		auto& pts = gen.getPoints();
		combinedPoints.insert(combinedPoints.end(), pts.begin(), pts.end());
		currentOffset += pts.size();
	}

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, combinedPoints.size() * sizeof(glm::vec4), combinedPoints.data(), GL_STATIC_DRAW);
}

auto ceilDiv = [](int a, int b) {
	return (a + b - 1) / b;
	};


void dispatchNoiseCompute(Shader& computeShader, GLuint targetTex, glm::ivec3 res, glm::ivec3 octaves, glm::ivec3 offsets, bool isShape) {
	computeShader.Activate();
	computeShader.SetUniform("numCells", octaves);
	computeShader.SetUniform("offsets", offsets);
	computeShader.SetUniform("isShape", isShape);
	computeShader.SetUniform("perlinScale", perlinScale);

	glBindImageTexture(0, targetTex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, (isShape ? ssboShape : ssboDetail));

	// Grupos de 8x8x8 conforme definido no local_size do shader
	glDispatchCompute(
		ceilDiv(res.x, 8),
		ceilDiv(res.y, 8),
		ceilDiv(res.z, 8)
	);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
}

void dispatchWeatherCompute(Shader& computeShader, GLuint targetTex)
{
	computeShader.Activate();
	glBindImageTexture(0, targetTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glDispatchCompute(ceilDiv(512, 8), ceilDiv(512, 8), 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
}

// Preview de textura 2D (weather map) -- nao precisa de slice
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

void generateAltitudePoints(int count, float minR, float maxR) {
	std::vector<AltitudePoint> points(count);
	std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
	std::uniform_real_distribution<float> dist(0.0f, 1.0f);
	std::uniform_real_distribution<float> posDist(0.0f, 1.0f);
	std::uniform_real_distribution<float> radDist(minR, maxR);

	for (auto& p : points) {
		p.position = glm::vec2(dist(rng), dist(rng));
		p.value = dist(rng);
		p.radius = radDist(rng); // distribuição separada para raio
	}

	glGenBuffers(1, &ssboAltitude);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboAltitude);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
		points.size() * sizeof(AltitudePoint),
		points.data(), GL_STATIC_DRAW);
}


unsigned int loadCubemap(std::vector<std::string> faces)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
			);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}