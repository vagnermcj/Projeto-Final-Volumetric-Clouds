#include <iostream>
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


#include"shaderClass.h"
#include"VAO.h"
#include"VBO.h"
#include"EBO.h"
#include"Floor.h"
#include"Cube.h"
#include"Camera.h"
#include"WorleyNoise3D.h"


// Configurações Globais
static GLuint quadID;
const unsigned int SCR_WIDTH = 800; //
const unsigned int SCR_HEIGHT = 800; //

// IDs de Recursos
GLuint shapeTexture, detailTexture;
GLuint ssboShape, ssboDetail;
GLuint quadVAO;

// Preview de Textura
GLuint previewFBO;
GLuint previewTex;  // textura 2D resultado
Shader* sliceShader;

// Parâmetros de Nuvem (Controlados pelo ImGui)
glm::ivec3 shapeOctaves(4, 8, 16);
glm::ivec3 detailOctaves(4, 8, 16);
float perlinScale = 1.0f;
bool needsUpdate = true;

// Estrutura para gerenciar pontos no SSBO
struct NoiseBufferData {
    std::vector<glm::vec3> allPoints;
    glm::ivec3 offsets;
};

// Protótipos
void initTextures();
void initTexturePreview();
void renderTexturePreview(Shader& sliceShader, GLuint tex3D, float slice, int channel);
void initScreenQuad();
void updateNoiseSSBO(GLuint ssbo, glm::ivec3 octaves, glm::ivec3& offsetsOut);
void dispatchNoiseCompute(Shader& computeShader, GLuint targetTex, glm::ivec3 res, glm::ivec3 octaves, glm::ivec3 offsets, bool isShape);
void drawScreenQuad();
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
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


	//Skybox Setup
	std::vector<std::string> faces = {
		"images/right.jpg", "images/left.jpg", "images/top.jpg", "images/bottom.jpg", "images/front.jpg", "images/back.jpg"
	};
	unsigned int cubemapTexture = loadCubemap(faces);

	// Inicializar Recursos
	initTextures();
	initTexturePreview();
	initScreenQuad();
	glGenBuffers(1, &ssboShape);
	glGenBuffers(1, &ssboDetail);

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
	glm::vec3 backgroundColor = glm::vec3(0.85, 0.9, 1.0); // cor do céu
	glm::vec3 cloudPosition = glm::vec3(1.0f, 0.0f, -2.0f);
	glm::vec3 cloudScale = glm::vec3(5.0f, 1.0f, 5.0f);
	glm::vec3 windDirection = glm::vec3(1.0f, 0.0f, 1.0f);
	glm::vec4 shapeNoiseWeights = glm::vec4(1.0f, 0.625f, 0.25f, 0.125f);

	float cloudMinCoverage = 0.5f;
	float weatherScale = 50.0f; // Peso da Detail Noise na formação das nuvens
	float scatteringCoefficient = 0.8f; // Coeficiente de espalhamento da luz nas nuvens
	float absorptionCoefficient = 0.05; //Coeficiente de absorção da luz
	float cloudStepSize = 0.5; // tamanho do passo para nuvens
	float cloudCoverage = 0.5f; // Cobertura de nuvens
	int cloudMaxSteps = 128; //Maximo de passos do raymarching
	int lightMaxSteps = 3;
	float windSpeed = 0.1f; //Velocidade do vento
	float atmosphereStart = 100.0f;
	float atmosphereHeight = 30.0f;
	float densityOffset = 0.0f;

	static float  previewSlice = 0.5f;
	static int    previewChannel = 0;
	static int    previewTarget = 0;


	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		// Clean the back buffer and assign the new color to it
		glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Atualização em Tempo Real (Compute Shader)
		if (needsUpdate) {
			glm::ivec3 sOff, dOff;
			// 1. Atualiza buffers de pontos
			updateNoiseSSBO(ssboShape, shapeOctaves, sOff);
			updateNoiseSSBO(ssboDetail, detailOctaves, dOff);

			// 2. Dispara geração da Shape Texture (128x32x128)
			dispatchNoiseCompute(noiseCompute, shapeTexture, glm::ivec3(128, 32, 128), shapeOctaves, sOff, true);

			// 3. Dispara geração da Detail Texture (32x32x32)
			dispatchNoiseCompute(noiseCompute, detailTexture, glm::ivec3(32, 32, 32), detailOctaves, dOff, false);

			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
			needsUpdate = false;
		}

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Cloud Generation");
		if (ImGui::DragInt3("Shape Octaves", glm::value_ptr(shapeOctaves), 1, 2, 64)) needsUpdate = true;
		if (ImGui::DragInt3("Detail Octaves", glm::value_ptr(detailOctaves), 1, 2, 64)) needsUpdate = true;
		if (ImGui::DragFloat("Perlin Scale", &perlinScale, 0.1f, 1.0f, 20.0f)) needsUpdate = true;
		ImGui::DragFloat4("Shape Weights", glm::value_ptr(shapeNoiseWeights), 0.01f, 0.0f, 2.0f);
		ImGui::DragFloat("Density Offset", &densityOffset, 0.01f, -1.0f, 1.0f);


		renderTexturePreview(*sliceShader,
			previewTarget == 0 ? shapeTexture : detailTexture,
			previewSlice, previewChannel);

		ImGui::SliderFloat("Slice Z", &previewSlice, 0.0f, 1.0f);
		ImGui::RadioButton("Shape", &previewTarget, 0);
		ImGui::SameLine();
		ImGui::RadioButton("Detail", &previewTarget, 1);
		ImGui::RadioButton("RGBA", &previewChannel, 0); ImGui::SameLine();
		ImGui::RadioButton("R", &previewChannel, 1); ImGui::SameLine();
		ImGui::RadioButton("G", &previewChannel, 2); ImGui::SameLine();
		ImGui::RadioButton("B", &previewChannel, 3); ImGui::SameLine();
		ImGui::RadioButton("A", &previewChannel, 4);

		// Agora sim passa a textura 2D renderizada
		ImGui::Image((ImTextureID)(intptr_t)previewTex, ImVec2(256, 256),
			ImVec2(0, 1),   // uv_min: começa do fundo
			ImVec2(1, 0));

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

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
		rayMarchingProgram.SetUniform("skybox", 3);
		
		  
		ImGui::Begin("Volumetric Clouds");

		ImGui::SeparatorText("General Setup");
		ImGui::ColorEdit3("Background Color", glm::value_ptr(backgroundColor));
		ImGui::DragFloat3("Cloud Position", glm::value_ptr(cloudPosition), 0.1f);
		ImGui::DragFloat3("Cloud Scale", glm::value_ptr(cloudScale), 0.1f);
		ImGui::DragFloat3("Wind Direction (Normalized)", glm::value_ptr(windDirection), 0.01f, -1.0f, 1.0f);
		ImGui::DragFloat("Wind Speed", &windSpeed, 0.005f, 0.0f);

		ImGui::SeparatorText("Atmosphere");

		ImGui::DragFloat("Atmosphere Start", &atmosphereStart, 0.5f);
		ImGui::DragFloat("Atmosphere Height", &atmosphereHeight, 0.5f, 0.0f);
		ImGui::DragFloat("Cloud Coverage", &cloudCoverage, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("Cloud Min Value", &cloudMinCoverage, 0.005f, 0.0f, 1.0f);
		ImGui::DragFloat("Weather Scale", &weatherScale, 1.0f, 0.1f);

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
		rayMarchingProgram.SetUniform("densityOffset", densityOffset);
		rayMarchingProgram.SetUniform("shapeNoiseWeights", shapeNoiseWeights);
		

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
		// Aloca espaço na GPU sem dados iniciais (NULL)
		glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, res.x, res.y, res.z, 0, GL_RGBA, GL_FLOAT, NULL);
		};

	create3DTex(shapeTexture, glm::ivec3(128, 32, 128), GL_RGBA32F);
	create3DTex(detailTexture, glm::ivec3(32, 32, 32), GL_RGBA32F);
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

	// Reutiliza seu próprio quad
	drawScreenQuad();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// Restaura viewport
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

void drawScreenQuad()
{
	glBindVertexArray(quadID);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

// Função para carregar Cubemap
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
			// Nota: Skyboxes geralmente são RGB (sem alpha)
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