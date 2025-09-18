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
GLfloat vertices[] =
{ //     COORDINATES      /       COLORS       //
	-0.5f, -0.5f,  0.5f,      1.0f, 0.0f, 0.0f,  // 0 - Red
	 0.5f, -0.5f,  0.5f,      0.0f, 1.0f, 0.0f,  // 1 - Green
	 0.5f,  0.5f,  0.5f,      0.0f, 0.0f, 1.0f,  // 2 - Blue
	-0.5f,  0.5f,  0.5f,      1.0f, 1.0f, 0.0f,  // 3 - Yellow
	-0.5f, -0.5f, -0.5f,      1.0f, 0.0f, 1.0f,  // 4 - Magenta
	 0.5f, -0.5f, -0.5f,      0.0f, 1.0f, 1.0f,  // 5 - Cyan
	 0.5f,  0.5f, -0.5f,      1.0f, 1.0f, 1.0f,  // 6 - White
	-0.5f,  0.5f, -0.5f,      0.0f, 0.0f, 0.0f   // 7 - Black
};

// Indices for vertices order
GLuint indices[] =
{
	// Front face
	0, 1, 2,
	0, 2, 3,
	// Back face
	4, 5, 6,
	4, 6, 7,
	// Right face
	1, 5, 6,
	1, 6, 2,
	// Left face
	0, 4, 7,
	0, 7, 3,
	// Top face
	3, 2, 6,
	3, 6, 7,
	// Bottom face
	0, 1, 5,
	0, 5, 4
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

	// Links VBO to VAO
	VAO1.LinkAttrib(VBO1, 0, 3, GL_FLOAT,6*sizeof(float), (void*)0);

	VAO1.LinkAttrib(VBO1, 1, 3, GL_FLOAT, 6*sizeof(float), (void*)(3 * sizeof(float)));
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

		camera.Inputs(window);
		camera.Matrix(45.0f, 0.1f, 100.0f, shaderProgram, "camMatrix");


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