#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>


class CloudTexture {
public:
	CloudTexture(int resolutionX, int resolutionY, int resolutionZ, glm::ivec3 octaves);
	GLuint MakeShape();
	GLuint MakeDetail();
	GLuint getTextureID() const { return _textureID; }
private:

	int _resolutionX, _resolutionY, _resolutionZ;
	GLuint _textureID;
	glm::ivec3 _octaves;

	std::vector<float> channelR;
	std::vector<float> channelG;
	std::vector<float> channelB;
	std::vector<float> channelA;
	std::vector<float> channelRGBA;
	std::vector<float> channelRGB;

	void MakeRGBA();
	void MakeRGB();
};

