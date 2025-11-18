#ifndef SHADER_CLASS_H
#define SHADER_CLASS_H

#include<glad/glad.h>
#include<vector>
#include<string>
#include<fstream>
#include<sstream>
#include<iostream>
#include<cerrno>
#include<glm/glm.hpp>

std::string get_file_contents(const char* filename);

class Shader
{
	public:
		// Reference ID of the Shader Program
		GLuint ID;
		// Constructor that build the Shader Program from 2 different shaders
		Shader(const char* vertexFile, const char* fragmentFile);
		void SetUniform(const std::string& varname, int x) const;
		void SetUniform(const std::string& varname, float x) const;
		void SetUniform(const std::string& varname, const glm::vec3& vet) const;
		void SetUniform(const std::string& varname, const glm::vec4& vet) const;
		void SetUniform(const std::string& varname, const glm::mat4& mat) const;
		void SetUniform(const std::string& varname, const std::vector<int>& x) const;
		void SetUniform(const std::string& varname, const std::vector<float>& x) const;
		void SetUniform(const std::string& varname, const std::vector<glm::vec3>& vet) const;
		void SetUniform(const std::string& varname, const std::vector<glm::vec4>& vet) const;
		void SetUniform(const std::string& varname, const std::vector<glm::mat4>& mat) const;
		// Activates the Shader Program
		void Activate();
		// Deletes the Shader Program
		void Delete();
	private:
		void compileErrors(unsigned int shader, const char* type);
};
#endif