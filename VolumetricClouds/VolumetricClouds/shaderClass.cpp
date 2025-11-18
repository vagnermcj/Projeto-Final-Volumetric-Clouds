#include"shaderClass.h"
#include <glm/gtc/type_ptr.hpp>

// Reads a text file and outputs a string with everything in the text file
std::string get_file_contents(const char* filename)
{
	std::ifstream in(filename, std::ios::binary);
	if (in)
	{
		std::string contents;
		in.seekg(0, std::ios::end);
		contents.resize(in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(&contents[0], contents.size());
		in.close();
		return(contents);
	}
	throw(errno);
}

// Constructor that build the Shader Program from 2 different shaders
Shader::Shader(const char* vertexFile, const char* fragmentFile)
{
	// Read vertexFile and fragmentFile and store the strings
	std::string vertexCode = get_file_contents(vertexFile);
	std::string fragmentCode = get_file_contents(fragmentFile);

	// Convert the shader source strings into character arrays
	const char* vertexSource = vertexCode.c_str();
	const char* fragmentSource = fragmentCode.c_str();

	// Create Vertex Shader Object and get its reference
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	// Attach Vertex Shader source to the Vertex Shader Object
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	// Compile the Vertex Shader into machine code
	glCompileShader(vertexShader);

	// Create Fragment Shader Object and get its reference
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	// Attach Fragment Shader source to the Fragment Shader Object
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	// Compile the Vertex Shader into machine code
	glCompileShader(fragmentShader);

	// Create Shader Program Object and get its reference
	ID = glCreateProgram();
	// Attach the Vertex and Fragment Shaders to the Shader Program
	glAttachShader(ID, vertexShader);
	glAttachShader(ID, fragmentShader);
	// Wrap-up/Link all the shaders together into the Shader Program
	glLinkProgram(ID);

	// Delete the now useless Vertex and Fragment Shader objects
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

}

// Activates the Shader Program
void Shader::Activate()
{
	glUseProgram(ID);
}

// Deletes the Shader Program
void Shader::Delete()
{
	glDeleteProgram(ID);
}

// Checks if the different Shaders have compiled properly
void Shader::compileErrors(unsigned int shader, const char* type)
{
	// Stores status of compilation
	GLint hasCompiled;
	// Character array to store error message in
	char infoLog[1024];
	if (type != "PROGRAM")
	{
		glGetShaderiv(shader, GL_COMPILE_STATUS, &hasCompiled);
		if (hasCompiled == GL_FALSE)
		{
			glGetShaderInfoLog(shader, 1024, NULL, infoLog);
			std::cout << "SHADER_COMPILATION_ERROR for:" << type << "\n" << infoLog << std::endl;
		}
	}
	else
	{
		glGetProgramiv(shader, GL_LINK_STATUS, &hasCompiled);
		if (hasCompiled == GL_FALSE)
		{
			glGetProgramInfoLog(shader, 1024, NULL, infoLog);
			std::cout << "SHADER_LINKING_ERROR for:" << type << "\n" << infoLog << std::endl;
		}
	}
}

void Shader::SetUniform(const std::string& varname, int x) const
{
	GLint loc = glGetUniformLocation(ID, varname.c_str());
	glUniform1i(loc, x);
}

void Shader::SetUniform(const std::string& varname, float x) const
{
	GLint loc = glGetUniformLocation(ID, varname.c_str());
	glUniform1f(loc, x);
}

void Shader::SetUniform(const std::string& varname, const glm::vec3& vet) const
{
	GLint loc = glGetUniformLocation(ID, varname.c_str());
	glUniform3fv(loc, 1, glm::value_ptr(vet));
}

void Shader::SetUniform(const std::string& varname, const glm::vec4& vet) const
{
	GLint loc = glGetUniformLocation(ID, varname.c_str());
	glUniform4fv(loc, 1, glm::value_ptr(vet));
}

void Shader::SetUniform(const std::string& varname, const glm::mat4& mat) const
{
	GLint loc = glGetUniformLocation(ID, varname.c_str());
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::SetUniform(const std::string& varname, const std::vector<int>& x) const
{
	GLint loc = glGetUniformLocation(ID, varname.c_str());
	glUniform1iv(loc, GLsizei(x.size()), x.data());
}

void Shader::SetUniform(const std::string& varname, const std::vector<float>& x) const
{
	GLint loc = glGetUniformLocation(ID, varname.c_str());
	glUniform1fv(loc, GLsizei(x.size()), x.data());
}

void Shader::SetUniform(const std::string& varname, const std::vector<glm::vec3>& vet) const
{
	GLint loc = glGetUniformLocation(ID, varname.c_str());
	glUniform3fv(loc, GLsizei(vet.size()), (float*)vet.data());
}

void Shader::SetUniform(const std::string& varname, const std::vector<glm::vec4>& vet) const
{
	GLint loc = glGetUniformLocation(ID, varname.c_str());
	glUniform4fv(loc, GLsizei(vet.size()), (float*)vet.data());
}

void Shader::SetUniform(const std::string& varname, const std::vector<glm::mat4>& mat) const
{
	GLint loc = glGetUniformLocation(ID, varname.c_str());
	glUniformMatrix4fv(loc, GLsizei(mat.size()), GL_FALSE, (float*)mat.data());
}