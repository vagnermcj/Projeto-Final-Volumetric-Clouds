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

static GLuint quadID;
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

	//Quad
	initScreenQuad();

	//Camera
	Camera camera(width, height, glm::vec3(0.0f, 0.0f, 2.0f));
	glm::vec3 lightDirection = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

	//ImGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 410 core");

	//FPS counters
	double prevTime = 0.0;
	double crntTime = 0.0;
	double timeDiff;
	unsigned int counter = 0;

	while (!glfwWindowShouldClose(window))
	{
		glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
		// Clean the back buffer and assign the new color to it
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		
		//FPS counting
		crntTime = glfwGetTime();
		timeDiff = crntTime - prevTime;
		counter++;

		if (timeDiff >= 1.0 / 30.0)
		{
			std::string FPS = std::to_string((1.0 / timeDiff) * counter);
			std::string ms = std::to_string((timeDiff / counter) * 1000);
			std::string newTitle = "Clouds - " + FPS + "FPS / " + ms + "ms";
			glfwSetWindowTitle(window, newTitle.c_str());

			prevTime = crntTime;
			counter = 0;
		}
		
		ImGui::Begin("Volumetric Clouds");
		ImGui::Text("Teste");
		ImGui::End();
		
		rayMarchingProgram.Activate();
		

		drawScreenQuad();
		//Camera Update
		camera.Inputs(window);
		camera.updateMatrix(45.0f, 0.1f, 100.0f); //Projection * view
		camera.Matrix(rayMarchingProgram, "camMatrix");
		glUniform3f(glGetUniformLocation(rayMarchingProgram.ID, "camPos"), camera.Position.x, camera.Position.y, camera.Position.z);
	

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(window);
		glfwPollEvents();
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