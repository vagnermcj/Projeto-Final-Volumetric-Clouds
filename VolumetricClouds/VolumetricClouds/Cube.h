#include<memory>
class Cube;
using CubePtr = std::shared_ptr<Cube>;

#include "VAO.h"
#include "EBO.h"

#ifndef CUBE_H
#define CUBE_H

class Cube
{
	VAO CubeVAO;
	GLuint indexes[6] =
	{
		0, 1, 2,
		0, 2, 3
	};
protected:
	Cube();
public:
	static CubePtr Make();
	virtual ~Cube();
	virtual void Draw();
};

#endif 
