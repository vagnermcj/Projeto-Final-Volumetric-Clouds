#include<memory>
class Floor;
using FloorPtr = std::shared_ptr<Floor>;

#include "VAO.h"

#ifndef FLOOR_H
#define FLOOR_H

class Floor
{
	VAO FloorVAO;
	GLuint indexes[6] =
	{
		0, 1, 2,
		0, 2, 3
	};
	protected:
		Floor(float area, float height);
	public:
		static FloorPtr Make(float area, float height);
		virtual ~Floor();
		virtual void Draw();
};

#endif 
