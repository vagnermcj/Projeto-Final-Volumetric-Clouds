#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include"shaderClass.h"
#include"VAO.h"
#include"VBO.h"
#include"EBO.h"
#include"Floor.h"
#include"Cube.h"
#include"Camera.h"
#include"CloudTexture.h"


static GLuint quadID;
static GLuint noiseTexture3D;
const unsigned int width = 800;
const unsigned int height = 800;

void initScreenQuad();
void drawScreenQuad();

int main()
{
	// Initialize GLFW
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(width, height, "Clouds", NULL, NULL);
	// Error check if the window fails to create
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}


	glfwMakeContextCurrent(window);
	gladLoadGL();
	glViewport(0, 0, width, height);

	//Shader shaderProgram("default.vert", "default.frag");
	Shader rayMarchingProgram("RayMarch.vert", "RayMarch.frag");

	//noise Setup
	CloudTexture shapeTexture(128, 32, 128, glm::ivec3(4, 8, 16));
	shapeTexture.MakeShape();


	//Quad
	initScreenQuad();

	

	//ImGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 410 core");

	//Camera
	Camera camera(width, height, glm::vec3(0.0f, 0.0f, 2.0f));
	

	//Parametros
	glm::vec3 lightDirection = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec3 backgroundColor = glm::vec3(0.85, 0.9, 1.0); // cor do céu
	glm::vec3 cloudPosition = glm::vec3(1.0f, 0.0f, -2.0f);
	glm::vec3 cloudScale = glm::vec3(5.0f, 1.0f, 5.0f);
	glm::vec3 windDirection = glm::vec3(1.0f, 0.0f, 1.0f);

	float absorptionCoefficient = 0.05; //Coeficiente de absorção da luz
	float cloudStepSize = 0.01; // tamanho do passo para nuvens
	float cloudCoverage = 0.5f; // Cobertura de nuvens
	int cloudMaxSteps = 248; //Maximo de passos do raymarching
	int lightMaxSteps = 3;
	float windSpeed = 0.1f; //Velocidade do vento



	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
		// Clean the back buffer and assign the new color to it
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		rayMarchingProgram.Activate();
		/*perlinNoise.Bind(0);
		worleyNoise.Bind(1);
		rayMarchingProgram.SetUniform("perlinTex", 0);
		rayMarchingProgram.SetUniform("worleyTex", 1);*/

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::ShowDemoWindow();
		
		  
		ImGui::Begin("Volumetric Clouds");

		ImGui::SeparatorText("General Setup");
		ImGui::ColorEdit3("Background Color", glm::value_ptr(backgroundColor));
		ImGui::DragFloat3("Cloud Position", glm::value_ptr(cloudPosition), 0.1f);
		ImGui::DragFloat3("Cloud Scale", glm::value_ptr(cloudScale), 0.1f);
		ImGui::DragFloat3("Wind Direction (Normalized)", glm::value_ptr(windDirection), 0.01f, -1.0f, 1.0f);
		ImGui::DragFloat("Wind Speed", &windSpeed, 0.01f, 0.0f);
		ImGui::DragFloat("Cloud Coverage", &cloudCoverage, 0.01f, 0.0f, 1.0f);

		ImGui::SeparatorText("Ray Marching");
		ImGui::DragInt("Cloud Max Steps", &cloudMaxSteps, 1, 0, ImGuiSliderFlags_AlwaysClamp);
		ImGui::DragFloat("Cloud Step Size", &cloudStepSize, 0.001f, 0.001f, ImGuiSliderFlags_AlwaysClamp);

		ImGui::SeparatorText("Lightning");
		ImGui::DragFloat3("Light Direction (Normalized)", glm::value_ptr(lightDirection), 0.01f, 0.0f, 1.0f);
		ImGui::ColorEdit3("Light Color", glm::value_ptr(lightColor));
		ImGui::DragFloat("Absorption Coefficient", &absorptionCoefficient, 0.001f, 0.0f, 1.0f);
		ImGui::DragInt("Light Max Steps", &lightMaxSteps, 1, 0, 5, "%d", ImGuiSliderFlags_AlwaysClamp);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

		ImGui::End();
		

		float t = glfwGetTime();
		//Passing Uniforms
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

		drawScreenQuad();
		//Camera Update
		camera.Inputs(window, &io);
		camera.updateMatrix(45.0f, 0.1f, 100.0f); //Projection * view
		camera.Matrix(rayMarchingProgram, "camMatrix");
		glUniform3f(glGetUniformLocation(rayMarchingProgram.ID, "camPos"), camera.Position.x, camera.Position.y, camera.Position.z);
	

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		
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

void drawScreenQuad()
{
	glBindVertexArray(quadID);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}