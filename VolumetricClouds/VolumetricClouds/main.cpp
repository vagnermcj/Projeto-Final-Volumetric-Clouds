#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


#include"shaderClass.h"
#include"VAO.h"
#include"VBO.h"
#include"EBO.h"
#include"Floor.h"
#include"Camera.h"


static FloorPtr sceneFloor;

const unsigned int width = 800;
const unsigned int height = 800;

// Vertices coordinates
GLfloat vertices[] = {
	//   POSIÇÃO           COR             NORMAL
	// Frente (z = +0.5)
	-0.5f, -0.5f,  0.5f,   1.0f,1.0f,1.0f,   0.0f, 0.0f, 1.0f,
	 0.5f, -0.5f,  0.5f,   1.0f,1.0f,1.0f,   0.0f, 0.0f, 1.0f,
	 0.5f,  0.5f,  0.5f,   1.0f,1.0f,1.0f,   0.0f, 0.0f, 1.0f,
	-0.5f,  0.5f,  0.5f,   1.0f,1.0f,1.0f,   0.0f, 0.0f, 1.0f,

	// Trás (z = -0.5)
	-0.5f, -0.5f, -0.5f,   1.0f,1.0f,1.0f,   0.0f, 0.0f,-1.0f,
	 0.5f, -0.5f, -0.5f,   1.0f,1.0f,1.0f,   0.0f, 0.0f,-1.0f,
	 0.5f,  0.5f, -0.5f,   1.0f,1.0f,1.0f,   0.0f, 0.0f,-1.0f,
	-0.5f,  0.5f, -0.5f,   1.0f,1.0f,1.0f,   0.0f, 0.0f,-1.0f,

	// Direita (x = +0.5)
	 0.5f, -0.5f, -0.5f,   1.0f,1.0f,1.0f,   1.0f, 0.0f, 0.0f,
	 0.5f, -0.5f,  0.5f,   1.0f,1.0f,1.0f,   1.0f, 0.0f, 0.0f,
	 0.5f,  0.5f,  0.5f,   1.0f,1.0f,1.0f,   1.0f, 0.0f, 0.0f,
	 0.5f,  0.5f, -0.5f,   1.0f,1.0f,1.0f,   1.0f, 0.0f, 0.0f,

	 // Esquerda (x = -0.5)
	 -0.5f, -0.5f, -0.5f,   1.0f,1.0f,1.0f,  -1.0f, 0.0f, 0.0f,
	 -0.5f, -0.5f,  0.5f,   1.0f,1.0f,1.0f,  -1.0f, 0.0f, 0.0f,
	 -0.5f,  0.5f,  0.5f,   1.0f,1.0f,1.0f,  -1.0f, 0.0f, 0.0f,
	 -0.5f,  0.5f, -0.5f,   1.0f,1.0f,1.0f,  -1.0f, 0.0f, 0.0f,

	 // Topo (y = +0.5)
	 -0.5f,  0.5f, -0.5f,   1.0f,1.0f,1.0f,   0.0f, 1.0f, 0.0f,
	  0.5f,  0.5f, -0.5f,   1.0f,1.0f,1.0f,   0.0f, 1.0f, 0.0f,
	  0.5f,  0.5f,  0.5f,   1.0f,1.0f,1.0f,   0.0f, 1.0f, 0.0f,
	 -0.5f,  0.5f,  0.5f,   1.0f,1.0f,1.0f,   0.0f, 1.0f, 0.0f,

	 // Fundo (y = -0.5)
	 -0.5f, -0.5f, -0.5f,   1.0f,1.0f,1.0f,   0.0f,-1.0f, 0.0f,
	  0.5f, -0.5f, -0.5f,   1.0f,1.0f,1.0f,   0.0f,-1.0f, 0.0f,
	  0.5f, -0.5f,  0.5f,   1.0f,1.0f,1.0f,   0.0f,-1.0f, 0.0f,
	 -0.5f, -0.5f,  0.5f,   1.0f,1.0f,1.0f,   0.0f,-1.0f, 0.0f
};


GLuint indices[] = {
	// Frente
	0, 1, 2,
	2, 3, 0,

	// Trás
	4, 5, 6,
	6, 7, 4,

	// Direita
	8, 9, 10,
	10, 11, 8,

	// Esquerda
	12, 13, 14,
	14, 15, 12,

	// Topo
	16, 17, 18,
	18, 19, 16,

	// Fundo
	20, 21, 22,
	22, 23, 20
};


int main()
{
	// Initialize GLFW
	glfwInit();

	// Tell GLFW what version of OpenGL we are using 
	// In this case we are using OpenGL 4.1
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	// Tell GLFW we are using the CORE profile
	// So that means we only have the modern functions
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Create a GLFWwindow object of 800 by 800 pixels, naming it "YoutubeOpenGL"
	GLFWwindow* window = glfwCreateWindow(width, height, "Window", NULL, NULL);
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



	// Generates Shader object using shaders defualt.vert and default.frag
	Shader shaderProgram("default.vert", "default.frag");


	// Generates Vertex Array Object and binds it
	VAO VAO1;
	VAO1.Bind();

	// Generates Vertex Buffer Object and links it to vertices
	VBO VBO1(vertices, sizeof(vertices));
	// Generates Element Buffer Object and links it to indices
	EBO EBO1(indices, sizeof(indices));


	VAO1.LinkAttrib(VBO1, 0, 3, GL_FLOAT, 9 * sizeof(float), (void*)0); //Coord
	VAO1.LinkAttrib(VBO1, 1, 3, GL_FLOAT, 9 * sizeof(float), (void*)(3 * sizeof(float))); //Color 
	VAO1.LinkAttrib(VBO1, 2, 3, GL_FLOAT, 9 * sizeof(float), (void*)(6 * sizeof(float))); //Normals

	// Unbind all to prevent accidentally modifying them
	VAO1.Unbind();
	VBO1.Unbind();
	EBO1.Unbind();

	sceneFloor = Floor::Make(100.0f, -1.0f);
	Camera camera(width, height, glm::vec3(0.0f, 0.0f, 2.0f));

	glEnable(GL_DEPTH_TEST); //Enable the depth calculation in the gl, so the triangles don't get buggy 

	// Main while loop
	while (!glfwWindowShouldClose(window))
	{
		glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
		// Clean the back buffer and assign the new color to it
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		shaderProgram.Activate();
		glm::vec3 lightDirection = glm::vec3(1.0f, 1.0f, 1.0f);
		glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		glUniform4f(glGetUniformLocation(shaderProgram.ID, "lightColor"), lightColor.x, lightColor.y, lightColor.z, lightColor.w);
		glUniform3f(glGetUniformLocation(shaderProgram.ID, "lightDirection"), lightDirection.x, lightDirection.y, lightDirection.z);
		camera.Inputs(window);
		camera.updateMatrix(45.0f, 0.1f, 100.0f);
		camera.Matrix(shaderProgram, "camMatrix");
		glUniform3f(glGetUniformLocation(shaderProgram.ID, "camPos"), camera.Position.x, camera.Position.y, camera.Position.z);
		
		VAO1.Bind();
		// Drawing Cube
		glDrawElements(GL_TRIANGLES, sizeof(indices)/sizeof(int), GL_UNSIGNED_INT, 0);

		sceneFloor->Draw();
		// Swap the back buffer with the front buffer
		glfwSwapBuffers(window);
		// Take care of all GLFW events
		glfwPollEvents();
	}



	// Delete all the objects we've created
	VAO1.Delete();
	VBO1.Delete();
	EBO1.Delete();
	shaderProgram.Delete();
	// Delete window before ending the program
	glfwDestroyWindow(window);
	// Terminate GLFW before ending the program
	glfwTerminate();
	return 0;
}