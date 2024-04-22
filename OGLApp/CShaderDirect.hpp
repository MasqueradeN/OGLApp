#pragma once
#include "../OGLApp/shaders/ShaderBase.h"

class CShaderDirect : public CShaderBase
{
public:
	CShaderDirect()
	{

	}

	~CShaderDirect()
	{

	}

	void initializeParticularShader() override
	{
		GLuint programme = getShaderProgram();
		positionLoc = glGetAttribLocation(programme, "in_Position");
		colorLoc = glGetAttribLocation(programme, "in_Color");
	}

	GLuint getPosLoc() override
	{
		return positionLoc;
	}

	GLuint getColorLoc() override
	{
		return colorLoc;
	}

private:
	GLuint positionLoc;
	GLuint colorLoc;
};