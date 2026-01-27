#include"CloudTexture.h"

#include"PerlinNoise3D.h"
#include"WorleyNoise3D.h"

CloudTexture::CloudTexture(int resolutionX, int resolutionY,
	int resolutionZ, glm::ivec3 octaves) : _resolutionX(resolutionX),
	_resolutionY(resolutionY), _resolutionZ(resolutionZ), _octaves(octaves)
{
	channelR.resize(_resolutionX * _resolutionY * _resolutionZ);
	channelG.resize(_resolutionX * _resolutionY * _resolutionZ);
	channelB.resize(_resolutionX * _resolutionY * _resolutionZ);
}

GLuint CloudTexture::MakeShape()
{
	channelA.resize(_resolutionX * _resolutionY * _resolutionZ);
	channelRGBA.resize(_resolutionX * _resolutionY * _resolutionZ * 4);
	PerlinNoise3D perlinNoise(_resolutionX, _resolutionY, _resolutionZ);
	WorleyNoise3D lowFreqWorley(_resolutionX, _resolutionY, _resolutionZ, _octaves[0]);
	WorleyNoise3D midFreqWorley(_resolutionX, _resolutionY, _resolutionZ, _octaves[1]);
	WorleyNoise3D highFreqWorley(_resolutionX, _resolutionY, _resolutionZ, _octaves[2]);
	perlinNoise.Generate();
	lowFreqWorley.Generate();
	midFreqWorley.Generate();
	highFreqWorley.Generate();

	channelR = perlinNoise.getData();
	channelG = lowFreqWorley.getData();
	channelB = midFreqWorley.getData();
	channelA = highFreqWorley.getData();

	MakeRGBA();

	glGenTextures(1, &_textureID);
	glBindTexture(GL_TEXTURE_3D, _textureID);

	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, _resolutionX, _resolutionY, _resolutionZ, 0, GL_RGBA, GL_FLOAT, channelRGBA.data());
	glBindTexture(GL_TEXTURE_3D, 0);
	return 1;
}

void CloudTexture::MakeRGBA()
{
	size_t totalPixels = _resolutionX * _resolutionY * _resolutionZ;

	for (size_t i = 0; i < totalPixels; i++)
	{
		channelRGBA[i * 4 + 0] = channelR[i];
		channelRGBA[i * 4 + 1] = channelG[i];
		channelRGBA[i * 4 + 2] = channelB[i];
		channelRGBA[i * 4 + 3] = channelA[i];
	}
}

// Fazer o bind